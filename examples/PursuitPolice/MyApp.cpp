#include "stdafx.h"
#include "MyApp.h"
#include "MyFrame.h"

#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>

// Es necesario enlazar con DbgHelp.lib
#pragma comment(lib, "dbghelp.lib")


bool MyApp::OnInit()
{
	MyFrame* frame = new MyFrame();
	frame->Show();
	
	return true;
}
