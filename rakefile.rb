require 'rake/clean'

CPU = "Cortex-M3"
MCU = "LPC1768"
PROJECT = "Mindroid"

# e.g. set KEIL_ARM_TOOLS=C:/PROGRA~1/Keil/ARM
def keil_arm_tools()
  return ENV['KEIL_ARM_TOOLS']
end

KEIL_ARM_TOOLS_DIR = "#{keil_arm_tools()}"
CC_DIR = "#{KEIL_ARM_TOOLS_DIR}/ARMCC/bin"
CC = "#{CC_DIR}/armcc.exe"
AS = "#{CC_DIR}/armasm.exe"
LD = "#{CC_DIR}/armlink.exe"
AR = "#{CC_DIR}/armar.exe"

RTX_DIR = "../RTX"

INCLUDE = "-I . -I #{KEIL_ARM_TOOLS_DIR}/CMSIS/Include -I #{RTX_DIR} -I #{KEIL_ARM_TOOLS_DIR}/RV31/INC -I #{KEIL_ARM_TOOLS_DIR}/INC/NXP/LPC17xx"
CCOPTS = "-c --cpu #{CPU} -D__MICROLIB -g -O3 --apcs=interwork #{INCLUDE}"
ASOPTS = "--cpu #{CPU} --pd \"__MICROLIB SETA 1\" -g --16 --apcs=interwork #{INCLUDE}"
AROPTS = ""

rule '.o' => ['.c'] do |t|
  sh "#{CC} #{CCOPTS} -o #{t.name} #{t.source}"
end

rule '.o' => ['.cpp'] do |t|
  sh "#{CC} #{CCOPTS} -o #{t.name} #{t.source}"
end

rule '.o' => ['.s'] do |t|
  sh "#{AS} #{ASOPTS} -o #{t.name} #{t.source}"
end

FileList['mindroid/**/*.cpp'].each do |src|
  task :ccobj => src.ext('.o')
end

task :compile => [:ccobj]

desc "Create archive"
task :archive do
  sh "#{AR} #{AROPTS} --create #{PROJECT}.lib mindroid/app/*.o mindroid/os/*.o mindroid/util/*.o"
end

CLEAN.include('mindroid/app/*.o').include('mindroid/os/*.o').include('mindroid/util/*.o').include("#{PROJECT}.lib")
CLOBBER.include("#{PROJECT}.axf")

desc "Build #{PROJECT}"
task PROJECT => [:compile, :archive]

task :default => [PROJECT]
