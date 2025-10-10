############################################################################
# Auto build script for Vivado project with HLS IP 2025.06.18 Naoki F., AIT
############################################################################
require 'FileUtils'
require 'rubygems'

VIVADO_DIR   = 'C:\Xilinx\Vivado\2022.1'
PROJ_NAME    = 'fpga_proj'
TOP_FUNCTION = 'search'
IP_VLNV      = 'xilinx.com:hls:search:1.0'
BASE_DIR     = 'C:\Doc\x\research\GPS\gpsacq\gpsacq_hls_triple'
PROJ_DIR     = BASE_DIR + "\\" + PROJ_NAME
IMPL_DIR     = PROJ_DIR + "\\" + PROJ_NAME + '.runs\impl_1'
GEN_DIR      = PROJ_DIR + "\\" + PROJ_NAME + ".gen"
BIT_FILE     = IMPL_DIR + '\design_1_wrapper.bit'
HWH_FILE     = GEN_DIR  + '\sources_1\bd\design_1\hw_handoff\design_1.hwh'
LOG_FILE     = 'vivado.log'

HLSLOG_DIR   = BASE_DIR + '\autobuild\log'
IP_DIR       = BASE_DIR + '\autobuild\ip_dir'
BIT_DIR      = BASE_DIR + '\autobuild\bit'

newpath = VIVADO_DIR + '\bin;' + VIVADO_DIR + '\lib\win64.o;' + ENV['PATH'];
newenv = {'PATH' => newpath, 'PROJECT_NAME' => PROJ_NAME,
          'PROJECT_BASE' => PROJ_DIR.gsub("\\", "/"), 'TOP_FUNCTION' => TOP_FUNCTION,
          'IP_VLNV' => IP_VLNV, 'XILINX_VIVADO' => VIVADO_DIR}

########################################################################
# main loop

Dir.glob("prn*", base: HLSLOG_DIR).each do |folder|
  folder_full = HLSLOG_DIR + "\\" + folder
  next if folder !~ /prn(\d+)_freq(\d+)_code(\d+)/
  prn = $1.to_i
  freq = $2.to_i
  code = $3.to_i
  ii_not_one = resource_over = false
  
  # HLS report
  open(folder_full + '/report/csynth.rpt', 'r') do |file|
    while line = file.gets
      if line =~ /\+ search[ \|-]+([0-9\.]+)\|\s+([0-9]+)/
        ii_not_one = true if $2.to_i > 20000
      end
    end
  end
  if ii_not_one
    puts "## skip: PRN %d, FREQ %d, CODE %d (because II /= 1)" % [prn, freq, code]
    next
  end

  # logic synthesis report
  open(folder_full + '/synth_rep.txt', 'r') do |file|
    while line = file.gets
      if line =~ /Slice LUTs[\* \|]+([0-9]+)/
        resource_over = true if $1.to_i > 30000
      end
    end
  end
  if resource_over
    puts "## skip: PRN %d, FREQ %d, CODE %d (because of resource over)" % [prn, freq, code]
    next
  end

  puts "## build: PRN %d, FREQ %d, CODE %d" % [prn, freq, code]
  puts
  sleep 1
  
  # extract zip
  FileUtils.rm_rf IP_DIR + '\export'
  FileUtils.mkdir IP_DIR + '\export'
  uz = `unzip -d \"#{IP_DIR}\\export\" \"#{HLSLOG_DIR}\\#{folder}\\export.zip\"`

  # run Vivado
  IO.popen([newenv, VIVADO_DIR + '\bin\vivado.bat',
           '-mode', 'batch', '-source', 'build_vivado.tcl',
           '-nojournal']) do |io|
    while line = io.gets
      if line =~ /^# [a-z]/ || line =~ /: Time \(s\)/
        puts "%2d|%2d|%2d|%s" % [prn, freq, code, line.chomp]
      end
    end
  end
  if ! $?.success?
    puts "!! failed to build the core (with status %d). stop." % $?
    exit 1
  end

  # copy relavant file
  begin
    FileUtils.mv LOG_FILE, folder_full + '\vivado_log.txt'
  rescue Errno::EACCES
    print '.'
    STDOUT.flush
    retry
  end
  FileUtils.cp BIT_FILE, BIT_DIR + ('\search%02d%02d%02d.bit' % [prn, freq, code])
  FileUtils.cp HWH_FILE, BIT_DIR + ('\search%02d%02d%02d.hwh' % [prn, freq, code])

end

########################################################################