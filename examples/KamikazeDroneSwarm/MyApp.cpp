//#include "stdafx.h"
#include "MyApp.h"
#include "MyFrame.h"
#include "MemoryLeaks.h"

#include <windows.h>
#include <DbgHelp.h>
#include <iostream>
#include <fstream>
#include <vector>

// Es necesario enlazar con DbgHelp.lib
#pragma comment(lib, "dbghelp.lib")


void WriteCallstack(PEXCEPTION_POINTERS pExceptionPtrs)
{
    std::ofstream archivo("crash_log.txt");
    if (!archivo.is_open()) return;

    archivo << "--- CRASH DETECTED ---\n";

    HANDLE proceso = GetCurrentProcess();
    SymInitialize(proceso, NULL, TRUE);

    void* stack[100];
    unsigned short frames = CaptureStackBackTrace(0, 100, stack, NULL);

    // Estructura para el nombre del símbolo
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    // Estructura para la línea y el fichero
    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement;

    for (unsigned int i = 0; i < frames; i++) {
        DWORD64 direccion = (DWORD64)(stack[i]);

        archivo << i << ": ";

        if (SymFromAddr(proceso, direccion, 0, symbol)) {
            archivo << symbol->Name;
        }
        else {
            archivo << "[Simbol not found]";
        }

        // INTENTO DE OBTENER FICHERO Y LÍNEA
        if (SymGetLineFromAddr64(proceso, direccion, &displacement, &line)) {
            archivo << " | File: " << line.FileName
                << " | Line: " << std::dec << line.LineNumber;
        }
        else {
            archivo << " | [No line info available (Missing PDBs?)]";
        }

        archivo << " - Dir: 0x" << std::hex << symbol->Address << "\n";
    }

    free(symbol);
    SymCleanup(proceso);
    archivo.close();
}

// This function executes when the program crashes
LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS pExceptionPtrs) 
{
	WriteCallstack(pExceptionPtrs);

    // Instructs the system to display the standard error message or close
	return EXCEPTION_EXECUTE_HANDLER;
}

bool MyApp::OnInit()
{
    // 1. Register the handler as soon as possible
	SetUnhandledExceptionFilter(ExceptionHandler);

	MyFrame* frame = new MyFrame();
	frame->Show();
	
	return true;
}
