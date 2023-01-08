#pragma once

#include <windows.h>

#include "module_scanner.h"
#include "../utils/threads_util.h"
#include "../utils/stats_analyzer.h"

namespace pesieve {

	//!  A report from the thread scan, generated by ThreadScanner
	class ThreadScanReport : public ModuleScanReport
	{
	public:
		static const DWORD THREAD_STATE_UNKNOWN = (-1);
		static const DWORD THREAD_STATE_WAITING = 5;

		ThreadScanReport(DWORD _tid)
			: ModuleScanReport(0, 0), 
			tid(_tid), thread_ip(0), protection(0),
			thread_state(THREAD_STATE_UNKNOWN), thread_wait_reason(0)
		{
		}

		const virtual void fieldsToJSON(std::stringstream &outs, size_t level, const pesieve::t_json_level &jdetails)
		{
			ModuleScanReport::_toJSON(outs, level);
			outs << ",\n";

			OUT_PADDED(outs, level, "\"thread_id\" : ");
			outs << std::dec << tid;
			outs << ",\n";
			OUT_PADDED(outs, level, "\"thread_ip\" : ");
			outs << "\"" << std::hex << thread_ip << "\"";
			outs << ",\n";
			if (thread_state != THREAD_STATE_UNKNOWN) {
				OUT_PADDED(outs, level, "\"thread_state\" : ");
				outs << std::dec << thread_state;
				outs << ",\n";

				if (thread_state == THREAD_STATE_WAITING) {
					OUT_PADDED(outs, level, "\"thread_wait_reason\" : ");
					outs << "\"" << translate_wait_reason(thread_wait_reason) << "\"";
					outs << ",\n";
				}
			}
			OUT_PADDED(outs, level, "\"protection\" : ");
			outs << "\"" << std::hex << protection << "\"";
			if (stats.isFilled()) {
				outs << ",\n";
				stats.toJSON(outs, level);
				outs << ",\n";
				area_info.toJSON(outs, level);
			}
		}

		const virtual bool toJSON(std::stringstream& outs, size_t level, const pesieve::t_json_level &jdetails)
		{
			OUT_PADDED(outs, level, "\"thread_scan\" : {\n");
			fieldsToJSON(outs, level + 1, jdetails);
			outs << "\n";
			OUT_PADDED(outs, level, "}");
			return true;
		}

		ULONGLONG thread_ip;
		DWORD tid;
		DWORD protection;
		DWORD thread_state;
		DWORD thread_wait_reason;
		util::AreaStats<BYTE> stats;
		util::AreaInfo area_info;
		bool entropy_filled;

	protected:
		static std::string translate_wait_reason(DWORD reson);
	};

	//!  A custom structure keeping a fragment of a thread context
	typedef struct _thread_ctx {
		bool is64b;
		ULONGLONG rip;
		ULONGLONG rsp;
		ULONGLONG rbp;
		ULONGLONG ret_addr; // the last return address on the stack (or the address of the first shellcode)
		bool is_managed; // does it contain .NET modules
	} thread_ctx;

	//!  A scanner for threads
	//!  Stack-scan inspired by the idea presented here: https://github.com/thefLink/Hunt-Sleeping-Beacons
	class ThreadScanner : public ProcessFeatureScanner {
	public:
		// neccessery to validly recognize stack frame
		static bool InitSymbols(HANDLE hProc);
		static bool FreeSymbols(HANDLE hProc);

		ThreadScanner(HANDLE hProc, bool _isReflection, const util::thread_info& _info, ModulesInfo& _modulesInfo, peconv::ExportsMapper* _exportsMap, pesieve::t_params _args)
			: ProcessFeatureScanner(hProc), isReflection(_isReflection), args(_args),
			info(_info), modulesInfo(_modulesInfo), exportsMap(_exportsMap)
		{
		}

		virtual ThreadScanReport* scanRemote();

	protected:

		bool isAddrInShellcode(ULONGLONG addr);
		bool resolveAddr(ULONGLONG addr);
		bool fetchThreadCtx(IN HANDLE hProcess, IN HANDLE hThread, OUT thread_ctx& c);
		size_t enumStackFrames(IN HANDLE hProcess, IN HANDLE hThread, IN LPVOID ctx, IN OUT thread_ctx& c);
		bool calcAreaStats(ThreadScanReport* my_report);
		bool isSuspiciousByStats(ThreadScanReport* my_report);
		bool reportSuspiciousAddr(ThreadScanReport* my_report, ULONGLONG susp_addr, thread_ctx& c);

		bool isReflection;
		const util::thread_info& info;
		ModulesInfo& modulesInfo;
		peconv::ExportsMapper* exportsMap;

		pesieve::t_params args;
	};

}; //namespace pesieve

