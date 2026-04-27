#include <stdio.h>

#include "../../src/dsp/midiverb_core.h"

int main(void) {
    mv_instance_t inst;
    mv_instance_init(&inst);
    printf("unit=%s program=%d (%s)\n",
           mv_unit_name(inst.unit), inst.program,
           mv_program_name(inst.unit, inst.program));
    printf("counts: midiverb=%d midifex=%d midiverb2=%d\n",
           mv_program_count(MV_UNIT_MIDIVERB),
           mv_program_count(MV_UNIT_MIDIFEX),
           mv_program_count(MV_UNIT_MIDIVERB2));
    return 0;
}
