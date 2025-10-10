cd $::env(PROJECT_BASE)
open_project $::env(PROJECT_NAME).xpr
update_ip_catalog -rebuild
report_ip_status -name ip_status
upgrade_ip -vlnv $::env(IP_VLNV) [get_ips design_1_$::env(TOP_FUNCTION)_0_0]
export_ip_user_files -of_objects [get_ips design_1_$::env(TOP_FUNCTION)_0_0] -no_script -sync -force -quiet
reset_run synth_1
launch_runs synth_1
wait_on_run synth_1
launch_runs impl_1
wait_on_run impl_1
launch_runs impl_1 -to_step write_bitstream
wait_on_run impl_1