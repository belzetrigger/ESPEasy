#!/usr/bin/env python
# ************************************
# espminiterm is serial terminal, that watches output from ESP.
# If there is stacktrace -> look up hex addresses in ELF file with addr2line and tell us some details
#
# For a full tool set look into from https://github.com/espressif/esp-idf with idf_monitor 
#
# requires:
#   pyserial (pip install pyserial || python -m pip install pyserial)
#
#
# based on sample on gist: https://gist.github.com/BBBSnowball/6a13c51fc4b89f0b65f18fe7342bc350
# adapt
#  - port to use
#  - path to xtensa-lx106-elf-addr2line
#  - path to firmware.elf
# 
# as this terminal blocks the connection, for uploading new build with platformio you have to exit this one first
# hints:
#   get connected devices 
#     python -m serial.tools.list_ports -v
#     you can use of cause also "platformio device list" taks as well
#
# fixed:
# - support python3
# - binary and binary regex output
# - windows
# todo
# - try catch on stack trace, partly it is not bin
# - show line number
# - maybe take build path from platformio.ini
# integreate somehow in platformio 
# ************************************
import serial
from serial.tools import miniterm
import subprocess, sys, re, os



class ESPMiniterm(miniterm.Miniterm):
  def handle_menu_key(self, c):
    if c == '\x15':  # ctrl-u
      self.upload_file()
    else:
      super(ESPMiniterm, self).handle_menu_key(c)

  # This will be called for menukey + ctrl-u.
  def upload_file(self):
    self._stop_reader()
    self.serial.close()
    while True:
      code = subprocess.call(["pio","run","-t","upload"])
      if code == 0:
        break
      print ("returncode: %d, press ctrl+u to try again, any other key to continue".format(code))
      try:
        c = self.console.getkey()
        if c == self.menu_character:
          c = self.console.getkey()
      except KeyboardInterrupt:
        c = '\x03'
      if c != '\x15':
        break
    self.serial.open()
    self._start_reader()

class ESPStackDecoder(miniterm.Transform):
  # https://github.com/esp8266/Arduino/blob/283eb97cd3f6dac691e8350a612021f1e1ccd8e3/doc/exception_causes.rst
  EXCEPTION_CAUSES = {
    0: "IllegalInstructionCause",
    1: "SyscallCause",
    2: "InstructionFetchErrorCause",
    3: "LoadStoreErrorCause",
    4: "Level1InterruptCause",
    5: "AllocaCause",
    6: "IntegerDivideByZeroCause",
    7: "Reserved for Tensilica",
    8: "PrivilegedCause",
    9: "LoadStoreAlignmentCause",
    10: "Reserved for Tensilica",
    11: "Reserved for Tensilica",
    12: "InstrPIFDataErrorCause",
    13: "LoadStorePIFDataErrorCause",
    14: "InstrPIFAddrErrorCause",
    15: "LoadStorePIFAddrErrorCause",
    16: "InstTLBMissCause",
    17: "InstTLBMultiHitCause",
    18: "InstFetchPrivilegeCause",
    19: "Reserved for Tensilica",
    20: "InstFetchProhibitedCause",
    21: "Reserved for Tensilica",
    22: "Reserved for Tensilica",
    23: "Reserved for Tensilica",
    24: "LoadStoreTLBMissCause",
    25: "LoadStoreTLBMultiHitCause",
    26: "LoadStorePrivilegeCause",
    27: "Reserved for Tensilica",
    28: "LoadProhibitedCause",
    29: "StoreProhibitedCause",
    30: "Reserved for Tensilica",
    31: "Reserved for Tensilica",
    32: "CoprocessornDisabled",
    33: "CoprocessornDisabled",
    34: "CoprocessornDisabled",
    35: "CoprocessornDisabled",
    36: "CoprocessornDisabled",
    37: "CoprocessornDisabled",
    38: "CoprocessornDisabled",
    39: "CoprocessornDisabled",
  }
  def __init__(self):
    miniterm.Transform.__init__(self)
    self.rxbuf = ""
    self.in_stack = False
  def rx(self, text):
    self.rxbuf += text
    if '\r' in self.rxbuf or '\n' in self.rxbuf:
      #lines = re.split("[\\r\\n]+", self.rxbuf)
      #self.rxbuf = lines[-1]
      #lines = lines[0:-1]
      #for line in lines:
      #  info = self.handle_line(line)
      #  if info and info != "":
      #    #TODO insert at the right position
      #    text += info
      prev = 0
      infos = []
      for m in re.finditer("(\\r\\n?|\\n)[\\r\\n]*", self.rxbuf):
        info = self.handle_line(self.rxbuf[prev:m.start()])
        if info:
          infos.append([m.end(1) - (len(self.rxbuf)-len(text)),
            re.sub(b"\\r\\n?|\\n", b"\r\n", info)])
        prev = m.end()
      infos.reverse()
      for info in infos:
        p = max(0, info[0])
        s = "{}".format(info[1])
        text = text[0:p] + s + text[p:]
      self.rxbuf = self.rxbuf[prev:]
    return text


  def handle_line(self, line):
    m = re.search("Exception \\(([0-9]*)\\):\s*$", line)
    if m:
      if int(m.group(1)) in self.EXCEPTION_CAUSES:
        return " -> Exception cause: %s\n" % (self.EXCEPTION_CAUSES[int(m.group(1))],)
      else:
        return " -> Exception cause: ???\n"
    m = re.match("^epc1=0x([0-9a-fA-F]{8,8}) ", line)
    if m:
      #return "address: %s\n" % (m.group(1),)
      return self.addr2line([m.group(1)])
    if line == ">>>stack>>>":
      self.in_stack = True
    elif line == "<<<stack<<<" or ":" not in line:
      self.in_stack = False
    if self.in_stack:
      info = ""
      addrs = []
      for m in re.finditer(" (40[0-2][0-9a-fA-F]{5,5})\\b", line):
        #info += "address: %s\n" % (m.group(1),)
        addrs.append("0x" + m.group(1))
      return self.addr2line(addrs)


  def addr2line(self, addrs):
    if len(addrs) == 0:
      return ""
    # standard nodemcu2
    #output = subprocess.check_output([os.environ["HOME"] + "/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-addr2line", "-aipfC", "-e", ".pioenvs/nodemcuv2/firmware.elf"] + addrs)
    
    #EspEasy uses .pio\build\dev_ESP8266_4M1M\firmware.elf
    output = subprocess.check_output([os.environ["HOME"] + "/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-addr2line", "-aipfC", "-e", ".pio/build/custom_ESP8266_4M1M/firmware.elf"] + addrs)
    return output


def printIntro():
  print("start now miniterm with stacktrace decoding")
  print("press ctrl+c to exit")
  print("do exit before upload new firmware\n")



printIntro()
miniterm.TRANSFORMATIONS['espstackdecoder'] = ESPStackDecoder

#miniterm.main(default_port="/dev/ttyUSB0", default_baudrate=115200)
#serial_instance = serial.serial_for_url(
#                #Linux "/dev/ttyUSB0",
                #windows
#                COM7,
#                115200,
#                do_not_open = True)
serial_instance = serial.Serial('COM7', 115200)

if not hasattr(serial_instance, 'cancel_read'):
    # enable timeout for alive flag polling if cancel_read is not available
    serial_instance.timeout = 1

#serial_instance.open()

miniterm = ESPMiniterm(
    serial_instance,
    echo=False,
    eol='crlf',
    filters=["espstackdecoder"])
# Python3 just char not unichar any more    
miniterm.exit_character = chr(0x03) #unichr(0x1d)
miniterm.menu_character = chr(0x14)
miniterm.raw = False
miniterm.set_rx_encoding('UTF-8')
miniterm.set_tx_encoding('UTF-8')

miniterm.start()

try:
    miniterm.join(True)
except KeyboardInterrupt:
    pass
sys.stderr.write("\n--- exit ---\n")
miniterm.join()
miniterm.close()
