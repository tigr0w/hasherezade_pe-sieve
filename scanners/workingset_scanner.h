#pragma once

#include <windows.h>
#include <psapi.h>
#include <map>

#include <peconv.h>
#include "module_scan_report.h"
#include "mempage_data.h"
#include "scan_report.h"

#include "../utils/format_util.h"
#include "../utils/workingset_enum.h"
#include "process_feature_scanner.h"
#include "process_details.h"

namespace pesieve {

	//!  A report from the working set scan, generated by WorkingSetScanner
	class WorkingSetScanReport : public ModuleScanReport
	{
	public:
		WorkingSetScanReport(HMODULE _module, size_t _moduleSize, t_scan_status status)
			: ModuleScanReport(_module, _moduleSize, status)
		{
			is_executable = false;
			is_listed_module = false;
			protection = 0;
			has_pe = false; //not a PE file
			has_shellcode = true;
			mapping_type = 0;
		}

		const virtual bool toJSON(std::stringstream &outs, size_t level, const pesieve::t_json_level &jdetails)
		{
			OUT_PADDED(outs, level, "\"workingset_scan\" : {\n");
			fieldsToJSON(outs, level + 1, jdetails);
			outs << "\n";
			OUT_PADDED(outs, level, "}");
			return true;
		}

		const virtual void fieldsToJSON(std::stringstream &outs, size_t level, const pesieve::t_json_level &jdetails)
		{
			ModuleScanReport::_toJSON(outs, level);
			outs << ",\n";
			OUT_PADDED(outs, level, "\"has_pe\" : ");
			outs << std::dec << has_pe;
			outs << ",\n";
			OUT_PADDED(outs, level, "\"has_shellcode\" : ");
			outs << std::dec << has_shellcode;
			if (!is_executable) {
				outs << ",\n";
				OUT_PADDED(outs, level, "\"is_executable\" : ");
				outs << std::dec << is_executable;
			}
			outs << ",\n";
			OUT_PADDED(outs, level, "\"is_listed_module\" : ");
			outs << std::dec << is_listed_module;
			outs << ",\n";
			OUT_PADDED(outs, level, "\"protection\" : ");
			outs << "\"" << std::hex << protection << "\"";
			outs << ",\n";
#ifdef CALC_PAGE_ENTROPY
			OUT_PADDED(outs, level, "\"entropy\" : ");
			outs << "\"" << std::dec << entropy << "\"";
			outs << ",\n";
#endif
			OUT_PADDED(outs, level, "\"mapping_type\" : ");
			outs << "\"" << translate_mapping_type(mapping_type) << "\"";
			if (mapping_type == MEM_IMAGE || mapping_type == MEM_MAPPED) {
				outs << ",\n";
				OUT_PADDED(outs, level, "\"mapped_name\" : ");
				outs << "\"" << pesieve::util::escape_path_separators(mapped_name) << "\"";
			}
		}

		bool is_executable;
		bool is_listed_module;
		bool has_pe;
		bool has_shellcode;
#ifdef CALC_PAGE_ENTROPY
		double entropy;
#endif
		DWORD protection;
		DWORD mapping_type;
		std::string mapped_name; //if the region is mapped from a file

	protected:
		static std::string translate_mapping_type(DWORD type)
		{
			switch (type) {
			case MEM_PRIVATE: return "MEM_PRIVATE";
			case MEM_MAPPED: return "MEM_MAPPED";
			case MEM_IMAGE: return "MEM_IMAGE";
			}
			return "unknown";
		}
	};


	//!  A scanner for detection of code implants in the process workingset.
	class WorkingSetScanner : public ProcessFeatureScanner {
	public:
		WorkingSetScanner(HANDLE _procHndl, process_details _proc_details, const util::mem_region_info _mem_region, pesieve::t_params _args, ProcessScanReport& _process_report)
			: ProcessFeatureScanner(_procHndl), pDetails(_proc_details),
			memRegion(_mem_region),
			args(_args),
			processReport(_process_report)
		{
		}

		virtual ~WorkingSetScanner() {}

		virtual WorkingSetScanReport* scanRemote();

	protected:
		bool scanImg(MemPageData& memPage);
		bool isScannedAsModule(MemPageData &memPageData);

		bool isExecutable(MemPageData &memPageData);
		bool isPotentiallyExecutable(MemPageData &memPageData, const t_data_scan_mode &mode);
		bool isCode(MemPageData &memPageData);
		WorkingSetScanReport* scanExecutableArea(MemPageData &memPageData);

		const process_details pDetails;
		const util::mem_region_info memRegion;

		ProcessScanReport& processReport;
		pesieve::t_params args;
	};

}; //namespace pesieve
