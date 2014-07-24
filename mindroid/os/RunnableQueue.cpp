/*
 * Copyright (C) 2014 Daniel Himmelein
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mindroid/os/RunnableQueue.h"
#include "mindroid/os/Runnable.h"
#include "mindroid/os/Looper.h"
#include "mindroid/os/Clock.h"

namespace mindroid {

RunnableQueue::RunnableQueue(Looper& looper) :
		Handler(looper),
		mMessageQueue(looper.myMessageQueue()) {
	Message::obtain(mMessage, *this, MSG_RUNNABLE);
}

RunnableQueue::~RunnableQueue() {
}

bool RunnableQueue::enqueueRunnable(Runnable& runnable, uint64_t execTimestamp) {
	if (execTimestamp == 0) {
		return false;
	}

	uint64_t nextExecTimestamp;
	{
		AutoLock autoLock(mLock);
		{
			AutoLock autoLock(runnable.mLock);
			if (runnable.mExecTimestamp != 0) {
				return false;
			}
			runnable.mExecTimestamp = execTimestamp;
		}
		Runnable* curRunnable = (Runnable*) mMessage.obj;
		if (curRunnable == NULL || execTimestamp < curRunnable->mExecTimestamp) {
			runnable.mNextRunnable = curRunnable;
			mMessage.obj = &runnable;
		} else {
			Runnable* prevRunnable = NULL;
			while (curRunnable != NULL && curRunnable->mExecTimestamp <= execTimestamp) {
				prevRunnable = curRunnable;
				curRunnable = curRunnable->mNextRunnable;
			}
			runnable.mNextRunnable = prevRunnable->mNextRunnable;
			prevRunnable->mNextRunnable = &runnable;
		}
		nextExecTimestamp = ((Runnable*) mMessage.obj)->mExecTimestamp;
	}
	mMessageQueue.removeMessage(this, &mMessage);
	mMessageQueue.enqueueMessage(mMessage, nextExecTimestamp);
	return true;
}

Runnable* RunnableQueue::dequeueRunnable() {
	Runnable* runnable = NULL;
	uint64_t nextExecTimestamp = 0;
	uint64_t now = Clock::monotonicTime();

	{
		AutoLock autoLock(mLock);
		Runnable* nextRunnable = (Runnable*) mMessage.obj;
		if (nextRunnable != NULL) {
			if (now >= nextRunnable->mExecTimestamp) {
				mMessage.obj = nextRunnable->mNextRunnable;
				runnable = nextRunnable;
				runnable->recycle();
			}

			Runnable* headRunnable = (Runnable*) mMessage.obj;
			if (headRunnable != NULL) {
				nextExecTimestamp = headRunnable->mExecTimestamp;
			}
		}
	}
	if (nextExecTimestamp != 0) {
		mMessageQueue.enqueueMessage(mMessage, nextExecTimestamp);
	}
	return runnable;
}

bool RunnableQueue::removeRunnable(const Runnable* runnable) {
	if (runnable == NULL) {
		return false;
	}

	bool foundRunnable = false;
	uint64_t nextExecTimestamp = 0;

	{
		AutoLock autoLock(mLock);
		Runnable* curRunnable = (Runnable*) mMessage.obj;
		// remove a matching runnable at the front of the runnable queue.
		if (curRunnable != NULL && curRunnable == runnable) {
			foundRunnable = true;
			Runnable* nextRunnable = curRunnable->mNextRunnable;
			mMessage.obj = nextRunnable;
			curRunnable->recycle();
			curRunnable = nextRunnable;
		}

		// remove a matching runnable after the front of the runnable queue.
		if (!foundRunnable) {
			while (curRunnable != NULL) {
				Runnable* nextRunnable = curRunnable->mNextRunnable;
				if (nextRunnable != NULL) {
					if (nextRunnable == runnable) {
						foundRunnable = true;
						Runnable* nextButOneRunnable = nextRunnable->mNextRunnable;
						nextRunnable->recycle();
						curRunnable->mNextRunnable = nextButOneRunnable;
						break;
					}
				}
				curRunnable = nextRunnable;
			}
		}

		Runnable* headRunnable = (Runnable*) mMessage.obj;
		if (headRunnable != NULL) {
			nextExecTimestamp = headRunnable->mExecTimestamp;
		}
	}
	if (nextExecTimestamp != 0) {
		mMessageQueue.enqueueMessage(mMessage, nextExecTimestamp);
	}

	return foundRunnable;
}

void RunnableQueue::handleMessage(const Message& message) {
	if (message.what == MSG_RUNNABLE) {
		Runnable* runnable = dequeueRunnable();
		if (runnable != NULL) {
			runnable->run();
		}
	}
}

} /* namespace mindroid */
