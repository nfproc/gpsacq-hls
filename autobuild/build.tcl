cd $::env(PROJECT_BASE)
open_project $::env(PROJECT_NAME)
open_solution $::env(SOLUTION_NAME)
# csim_design
csynth_design
export_design -format ip_catalog -flow syn
exit