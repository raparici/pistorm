/*
 * Copyright 2020 Claude Schwarz
 *
 * Niklas Ekström 2020 - reorganized source code
 */
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "m68k.h"
#include "psconf.h"
#include "ps_protocol.h"
#include "ps_mappings.h"
#include "ps_kickstart.h"
#include "gayle.h"

int use_gayle_emulation;

static void parse_args(int argc, char *argv[]) {
  use_gayle_emulation = 1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--disable-gayle") == 0) {
      use_gayle_emulation = 0;
    }
  }
}

int main(int argc, char *argv[]) {
  printf("PiStorm 68k accelerator\n");
  printf("Copyright 2020 Claude Schwarz\n");

  parse_args(argc, argv);

  const struct sched_param priority = {99};
  sched_setscheduler(0, SCHED_FIFO, &priority);
  mlockall(MCL_CURRENT);

  init_mappings();

  ps_setup_protocol();
  ps_reset_state_machine();
  ps_pulse_reset();

  usleep(1500);

  m68k_init();
  m68k_set_cpu_type(M68K_CPU_TYPE_68020);
  m68k_pulse_reset();

  int res = init_kickstart();
  if (res == 0)
    m68k_set_reg(M68K_REG_PC, KICK_BASE + 2);

  if (use_gayle_emulation)
    init_gayle("hd0.img");

  while (1) {
    m68k_execute(300);

    if (!ps_get_aux1()) {
      unsigned int status = ps_read_status_reg();
      m68k_set_irq((status & 0xe000) >> 13);
    } else if (check_gayle_irq()) {
      PAULA_SET_IRQ(3); // IRQ 3 = INT2
      m68k_set_irq(2);
    } else {
      m68k_set_irq(0);
    }
  }

  return 0;
}

void cpu_pulse_reset() {
  ps_pulse_reset();
}
