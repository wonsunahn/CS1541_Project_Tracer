/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*! @file
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include "pin.H"
#include "trace.h"
#include <iostream>
#include <fstream>
#include <iomanip>
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::ios;
using std::setw;
using std::string;
/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

std::ofstream TraceFile;

const CHAR * ROI_BEGIN = "__roi_begin__";
const CHAR * ROI_END = "__roi_end__";
bool isROI = false;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB< string > KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "tracer.out", "specify output file name");
KNOB< string > KnobTraceFile(KNOB_MODE_WRITEONCE, "pintool", "t", "tracer.tr", "specify trace file name");
KNOB< BOOL > KnobValues(KNOB_MODE_WRITEONCE, "pintool", "values", "1", "Output memory values reads and written");

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

static INT32 Usage()
{
    cerr << "This tool produces a memory address trace.\n"
            "For each (dynamic) instruction reading or writing to memory the the ip and ea are recorded\n"
            "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

static VOID EmitMem(VOID* ea, INT32 size)
{
    if (!KnobValues) return;

    switch (size)
    {
        case 0:
            TraceFile << setw(1);
            break;

        case 1:
            TraceFile << static_cast< UINT32 >(*static_cast< UINT8* >(ea));
            break;

        case 2:
            TraceFile << *static_cast< UINT16* >(ea);
            break;

        case 4:
            TraceFile << *static_cast< UINT32* >(ea);
            break;

        case 8:
            TraceFile << *static_cast< UINT64* >(ea);
            break;

        default:
            TraceFile.unsetf(ios::showbase);
            TraceFile << setw(1) << "0x";
            for (INT32 i = 0; i < size; i++)
            {
                TraceFile << static_cast< UINT32 >(static_cast< UINT8* >(ea)[i]);
            }
            TraceFile.setf(ios::showbase);
            break;
    }
}

static VOID RecordMem(VOID* ip, CHAR r, VOID* addr, INT32 size, BOOL isPrefetch)
{
    // Return if not in ROI
    if(!isROI) return;

    TraceFile << ip << ": " << r << " " << setw(2 + 2 * sizeof(ADDRINT)) << addr << " " << dec << setw(2) << size << " " << hex
              << setw(2 + 2 * sizeof(ADDRINT));
    if (!isPrefetch) EmitMem(addr, size);
    TraceFile << endl;

    instruction inst;
    if (r == 'R') {inst.type = ti_LOAD;}
    else if (r == 'W') {inst.type = ti_STORE;}
    else {assert(false);}

    inst.PC = (uintptr_t) ip;
    inst.Addr = (uintptr_t) addr;
    write_trace(inst);
}

static VOID* WriteAddr;
static INT32 WriteSize;

static VOID RecordWriteAddrSize(VOID* addr, INT32 size)
{
    // Return if not in ROI
    if(!isROI) return;

    WriteAddr = addr;
    WriteSize = size;
}

static VOID RecordMemWrite(VOID* ip) { RecordMem(ip, 'W', WriteAddr, WriteSize, false); }

VOID Instruction(INS ins, VOID* v)
{
    // instruments loads using a predicated call, i.e.
    // the call happens iff the load will be actually executed

    if (INS_IsMemoryRead(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_INST_PTR, IARG_UINT32, 'R', IARG_MEMORYREAD_EA,
                                 IARG_MEMORYREAD_SIZE, IARG_BOOL, INS_IsPrefetch(ins), IARG_END);
    }

    if (INS_HasMemoryRead2(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem, IARG_INST_PTR, IARG_UINT32, 'R', IARG_MEMORYREAD2_EA,
                                 IARG_MEMORYREAD_SIZE, IARG_BOOL, INS_IsPrefetch(ins), IARG_END);
    }

    // instruments stores using a predicated call, i.e.
    // the call happens iff the store will be actually executed
    if (INS_IsMemoryWrite(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordWriteAddrSize, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
                                 IARG_END);

        if (INS_IsValidForIpointAfter(ins))
        {
            INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)RecordMemWrite, IARG_INST_PTR, IARG_END);
        }
        if (INS_IsValidForIpointTakenBranch(ins))
        {
            INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)RecordMemWrite, IARG_INST_PTR, IARG_END);
        }
    }
}

VOID StartROI()
{
    isROI = true;
}

VOID StopROI()
{
    isROI = false;
}

VOID Routine(RTN rtn, VOID *v)
{
    // Get routine name
    const CHAR * name = RTN_Name(rtn).c_str();

    // Do a string search of the markers instead of an exact comparison to
    // deal with C++ name mangling.
    if(strstr(name,ROI_BEGIN) != NULL) {
        // Start tracing after ROI begin exec
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)StartROI, IARG_END);
        RTN_Close(rtn);
    } else if (strstr(name,ROI_END) != NULL) {
        // Stop tracing before ROI end exec
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)StopROI, IARG_END);
        RTN_Close(rtn);
    }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID* v)
{
    TraceFile << "#eof" << endl;

    TraceFile.close();
    trace_uninit();
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    string trace_header = string("#\n"
                                 "# Memory Access Trace Generated By Pin\n"
                                 "#\n");

    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    TraceFile.open(KnobOutputFile.Value().c_str());
    TraceFile.write(trace_header.c_str(), trace_header.size());
    TraceFile.setf(ios::showbase);

    trace_init(KnobTraceFile.Value().c_str(), "wb");

    RTN_AddInstrumentFunction(Routine, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns

    PIN_StartProgram();

    RecordMemWrite(0);
    RecordWriteAddrSize(0, 0);

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
