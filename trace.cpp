/** Code by @author Wonsun Ahn
 * 
 * Functions to read and write the trace file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include "trace.h"

FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static instruction *trace_buf;

void trace_init(const char *trace_file_name, const char *mode)
{
	trace_fd = fopen(trace_file_name, mode);
	if (trace_fd == NULL) {
		exit(-1);
	}

	trace_buf = (instruction *) malloc(sizeof(instruction) * TRACE_BUFSIZE);

	if (trace_buf == NULL) {
		exit(-1);
	}

	trace_buf_ptr = 0;
	trace_buf_end = 0;
}

void trace_uninit()
{
	free(trace_buf);
	fclose(trace_fd);
}

int trace_get_item(instruction **item)
{
	int n_items;

	if (trace_buf_ptr == trace_buf_end) {	/* if no more unprocessed items in the trace buffer, get new data  */
		n_items = fread(trace_buf, sizeof(instruction), TRACE_BUFSIZE, trace_fd);
		if (!n_items) return 0;				/* if no more items in the file, we are done */

		trace_buf_ptr = 0;
		trace_buf_end = n_items;			/* n_items were read and placed in trace buffer */
	}

	*item = &trace_buf[trace_buf_ptr];	/* read a new trace item for processing */
	trace_buf_ptr++;

	return 1;
}

int write_trace(instruction item)
{
	int n_items;

	n_items = fwrite(&item, sizeof(instruction), 1, trace_fd);
	if (!n_items) return 0;				/* if no more items in the file, we are done */

	return 1;
}

char* get_instruction_string(dynamic_inst dinst, Format format)
{
  static char buffer[100];
  instruction inst = dinst.inst;
  const char *name;

  switch(inst.type) {
    case ti_NOP:
      name = "NOP";
      break;
    case ti_RTYPE:
      name = "RTYPE";
      break;
    case ti_ITYPE:
      name = "ITYPE";
      break;
    case ti_LOAD:
      name = "LOAD";
      break;
    case ti_STORE:
      name = "STORE";
      break;
    case ti_BRANCH:
      name = "BRANCH";
      break;
    case ti_JTYPE:
      name = "JTYPE";
      break;
    case ti_SPECIAL:
      name = "SPECIAL";
      break;
    case ti_JRTYPE:
      name = "JRTYPE";
      break;
    default:
      exit(-1);
  }
  switch(format) {
    case SHORT_FORM:
      snprintf(buffer, 100, "%7s: (Seq: %8d)(Regs: %3d, %3d, %3d)", name, dinst.seq, inst.dReg, inst.sReg_a, inst.sReg_b);
      break;
    case LONG_FORM:
      snprintf(buffer, 100, "%7s: (Seq: %8d)(Regs: %3d, %3d, %3d)(Addr: %u)(PC: %u)", name, dinst.seq, inst.dReg, inst.sReg_a, inst.sReg_b, inst.Addr, inst.PC);
      break;
    case ADDR_ONLY:
      snprintf(buffer, 100, "%s: (Seq: %8d)(Addr: %u)(PC: %u)", name, dinst.seq, inst.Addr, inst.PC);
      break;
  }
  return buffer;
}
