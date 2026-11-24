/* lab_experiment — perturbation / cross-model experiments + reporting.
 * Phase 1 declares only the CLI entries; experiments land in Phase 3. */
#ifndef CM2008_LAB_EXPERIMENT_H
#define CM2008_LAB_EXPERIMENT_H
int lab_cmd_experiment(int argc, char **argv);
int lab_cmd_trace(int argc, char **argv);
int lab_cmd_report(int argc, char **argv);
#endif
