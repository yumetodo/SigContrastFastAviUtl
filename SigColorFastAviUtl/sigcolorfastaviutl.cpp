#define NOMINMAX
#include <Windows.h>
#include "filter.h" //please set this to AviUtl SDK's filter.h
#include "SigmoidTable.hpp"
#include "RSigmoidTable.hpp"
#include "thread.hpp"
#include "color_cvt.hpp"
#include "filter_helper.hpp"
#ifdef USECLOCK
#include <chrono>
#include <ctime>
#include <ratio>
#include <string>
#include <fstream>
#include <deque>
namespace ch = std::chrono;
#endif


#ifdef _DEBUG
#define PLUGIN_NAME_SCON "SContrast DEBUG"
#define VERSION_STR_SCON "SContrast(DEBUG) v0.3 by MaverickTse and Yumetodo"
#define PLUGIN_NAME_SDCON "SDeContrast DEBUG"
#define VERSION_STR_SDCON "SDeContrast(DEBUG) v0.3 by MaverickTse and Yumetodo"
#else
#define PLUGIN_NAME_SCON "SContrast"
#define VERSION_STR_SCON "SContrast v0.3 by MaverickTse and Yumetodo"
#define PLUGIN_NAME_SDCON "SDeContrast"
#define VERSION_STR_SDCON "SDeContrast v0.3 by MaverickTse and Yumetodo"
#endif




// Define sliders
#define	TRACK_N	2 //	slider count
// slider name
char* en_name[] = { "Midtone", "Strength" };


int		track_default[] = { 50, 5 };	//	default values
int		track_s[] = { 0, 1 };	//	minimum values
int		track_e[] = { 100, 30 };	//	maximum values

											
#ifdef USECLOCK
static inline void disable_echo_benchmark(FILTER *fp) {
	filter_proxy fc(fp);
	if (fc[check::disable_benchmark_during_export]) {
		fc.disable_benchmark();
		fc.notify_update_window();
	}
}
#	define	CHECK_N	7														//	total number of check box and button
#else
#	define	CHECK_N	4														//	total number of check box and button
#endif
char	*check_name_en[CHECK_N] = { 
	"Y", "R", "G", "B"
#ifdef USECLOCK
	, "Echo benchmark", "Save benchmark", "Disable benchmark during export"
#endif
};				//	label name
int		check_default[CHECK_N] = {
	1, 0, 0, 0
#ifdef USECLOCK
	,1,0,1
#endif
};				//	for checkbox: 0(unchecked) or 1(checked); for button: must be -1
static_assert((sizeof(check_name_en) / sizeof(*check_name_en)) == (sizeof(check_default) / sizeof(*check_default)), "error");

namespace sigmoid_contrast {
	static SigmoidTable ST;
#ifdef USECLOCK
	std::deque<ch::nanoseconds::rep> logbuf;
#endif
	BOOL init(FILTER* /*fp*/) noexcept { return TRUE; }
	BOOL exit(FILTER* /*fp*/)
	{
		//DO NOT PUT MessageBox here, crash the application!
		//MessageBox(NULL, "func_exit invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
#ifdef USECLOCK
		if (!logbuf.empty()) {
			std::ofstream logfile_sc("log_sc.csv");
			for (auto&& i : logbuf) {
				logfile_sc << "SCon," << i << std::endl;
			}
		}
#endif
		return TRUE;
	}
	BOOL update(FILTER* fp, int status)
	{
		filter_proxy fc(fp);
		switch (status){
			case FILTER_UPDATE_MIDTONE_TRACK:
			case FILTER_UPDATE_STRENGTH_TRACK:
				ST.change_param(fc[track::midtone] / 100.0f, static_cast<float>(fc[track::strength]));
				break;
			case FILTER_UPDATE_Y_CHECK:
				fc.set_rgb(!fc[check::Y]);
				break;
			case FILTER_UPDATE_R_CHECK:
			case FILTER_UPDATE_G_CHECK:
			case FILTER_UPDATE_B_CHECK:
				if (fc[check::Y]){
					fc[check::Y] = 0;
				}
				else if (fc.none_of(check::R, check::G, check::B)){
					fc[check::Y] = 1;
				}
				break;
#ifdef USECLOCK
			case FILTER_UPDATE_ECHO_BENCHMARK_CHECK:
				if (fc[check::echo_benchmark]) SetWindowText(fc.window_handle(), PLUGIN_NAME_SCON);
				break;
#endif
			default:
				//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
				break;
		}
		fc.notify_update_window();

		return TRUE;
	}
	BOOL proc(FILTER* fp, FILTER_PROC_INFO* fpip) // This is the main image manipulation function
	{
		const filter_proxy fc(fp);
#ifdef USECLOCK
		ch::time_point<ch::steady_clock> start_con;
		if (fc.any_of(check::echo_benchmark, check::save_benchmark)) start_con = ch::steady_clock::now();
#endif

		ST.change_param(fc[track::midtone] / 100.0f, static_cast<float>(fc[track::strength]));

		if (fc.none_of(check::R, check::G, check::B)){
			/* Scan Y channel data */
			parallel::par_for(fpip->h, [fpip](int begin, int end) {
				for (int r = begin; r < end; r++)
				{
					for (int c = 0; c < fpip->w; c++)
					{
						PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
						px->y = ST.lookup(px->y);
					}
				}
			});
		}
		else { //RGB mode
			parallel::par_for(fpip->h, [fpip, fc](int begin, int end) {
				float buf[4] = { 0 };
				for (int r = begin; r < end; r++){
					for (int c = 0; c < fpip->w; c++){
						PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
						color_cvt::yc2rgb(buf, px);
						// transform each channel is needed
						if (fc[check::B]){
							buf[1] = static_cast<float>(ST.lookup(buf[1]));
						}
						if (fc[check::G]){
							buf[2] = static_cast<float>(ST.lookup(buf[2]));
						}
						if (fc[check::R]){
							buf[3] = static_cast<float>(ST.lookup(buf[3]));
						}
						color_cvt::rgb2yc(px, buf);
					}
				}
			});
		}
#ifdef USECLOCK
		if (fc.any_of(check::echo_benchmark, check::save_benchmark)){
			const auto end_con = ch::steady_clock::now();
			const auto elapsed = end_con - start_con;
			if (fc[check::echo_benchmark]) {
				using namespace std::chrono_literals;//UDLs : ms
				static auto last_echo_time = end_con;
				if (last_echo_time == end_con || last_echo_time + 150ms < end_con) {
					const auto elapsed_s = std::to_string(ch::duration_cast<ch::microseconds>(elapsed).count());
					SetWindowText(fc.window_handle(), ("SCon:" + elapsed_s + "micro sec. @" + std::to_string(fpip->w) + "x" + std::to_string(fpip->h)).c_str());
					fc.notify_update_window();
					last_echo_time = end_con;
				}
			}
			if (fc[check::save_benchmark]) {
				logbuf.push_back(ch::duration_cast<ch::nanoseconds>(elapsed).count());
			}
		}
#endif
		return TRUE; //TRUE to update frame image. FALSE to skip refresh.
	}
	BOOL WndProc(HWND /*hwnd*/, UINT message, WPARAM /*wparam*/, LPARAM /*lparam*/, void* /*editp*/, FILTER* fp)
	{
#ifdef USECLOCK
		switch (message){
			case WM_FILTER_EXPORT:
			case WM_FILTER_SAVE_START:
				disable_echo_benchmark(fp);
				break;
			//case WM_FILTER_MAIN_MOUSE_DOWN:
			//case WM_FILTER_MAIN_MOUSE_MOVE:
			//case WM_FILTER_FILE_CLOSE:
			default:
				break;
		}
#endif
		return FALSE;
	}
	BOOL save_start(FILTER* fp, int /*s*/, int /*e*/, void* /*editp*/)
	{
#ifdef USECLOCK
		disable_echo_benchmark(fp);
#endif
		return TRUE;
	}
	BOOL save_end(FILTER* /*fp*/, void* /*editp*/)
	{
#ifdef USECLOCK
		if (!logbuf.empty()) {
			std::ofstream logfile_sc("log_sc.csv");
			for (auto&& i : logbuf) {
				logfile_sc << "SCon," << i << std::endl;
			}
		}
#endif
		return TRUE;
	}

	static FILTER_DLL filter_info_en = {               // English UI filter info
		FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_PRIORITY_LOWEST,	//	filter flags, use bitwise OR to add more
		0, 0,							//	dialogbox size
		PLUGIN_NAME_SCON,				//	Filter plugin name
		TRACK_N,						//	トラックバーの数 (0なら名前初期値等もNULLでよい)
		en_name,						//	slider label names in English
		track_default,					//	トラックバーの初期値郡へのポインタ
		track_s, track_e,				//	トラックバーの数値の下限上限 (NULLなら全て0～256)
		CHECK_N,						//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
		check_name_en,					//	チェックボックスの名前郡へのポインタ
		check_default,					//	チェックボックスの初期値郡へのポインタ
		proc,			//	main filter function, use NULL to skip
		init,			//	initialization function, use NULL to skip
		exit,			//	on-exit function, use NULL to skip
		update,		//	invokes when when settings changed. use NULL to skip
		WndProc,		//	for capturing dialog's control messages. Essential if you use button or auto uncheck some checkboxes.
		NULL, NULL,						//	Reserved. Do not use.
		NULL,							//  pointer to extra data when FILTER_FLAG_EX_DATA is set
		NULL,							//  extra data size
		VERSION_STR_SCON,
		//  pointer or c-string for full filter info when FILTER_FLAG_EX_INFORMATION is set.
		save_start,	//	invoke just before saving starts. NULL to skip
		save_end,		//	invoke just after saving ends. NULL to skip
	};
}
namespace sigmoid_decontrast {
	static RSigmoidTable RST;
#ifdef USECLOCK
	std::deque<ch::nanoseconds::rep> logbuf;
#endif

	BOOL init(FILTER* /*fp*/) noexcept { return TRUE; }
	BOOL exit(FILTER* /*fp*/)
	{
		//DO NOT PUT MessageBox here, crash the application!
#ifdef USECLOCK
		if (!logbuf.empty()) {
			std::ofstream logfile_sd("log_sd.csv");
			for (auto&& i : logbuf) {
				logfile_sd << "SDeCon," << i << std::endl;
			}
		}
#endif
		return TRUE;
	}
	BOOL update(FILTER* fp, int status)
	{
		filter_proxy fc(fp);
		switch (status){
			case FILTER_UPDATE_MIDTONE_TRACK:
			case FILTER_UPDATE_STRENGTH_TRACK:
				RST.change_param(fc[track::midtone] / 100.0f, static_cast<float>(fc[track::strength]));
				break;
			case FILTER_UPDATE_Y_CHECK:
				fc.set_rgb(!fc[check::Y]);
				break;
			case FILTER_UPDATE_R_CHECK:
			case FILTER_UPDATE_G_CHECK:
			case FILTER_UPDATE_B_CHECK:
				if (fc[check::Y]){
					fc[check::Y] = 0;
				}
				else if (fc.none_of(check::R, check::G, check::B)) {
					fc[check::Y] = 1;
				}
				break;
#ifdef USECLOCK
			case FILTER_UPDATE_ECHO_BENCHMARK_CHECK:
				if (!fc[check::echo_benchmark]) SetWindowText(fc.window_handle(), PLUGIN_NAME_SDCON);
				break;
#endif
			default:
				//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
				break;
		}
		fc.notify_update_window();
		return TRUE;
	}
	BOOL proc(FILTER* fp, FILTER_PROC_INFO* fpip) // This is the main image manipulation function
	{
		const filter_proxy fc(fp);
#ifdef USECLOCK
		ch::time_point<ch::steady_clock> start_sd;
		if (fc.any_of(check::echo_benchmark, check::save_benchmark)) start_sd = ch::steady_clock::now();
#endif

		/* Create a Reverse sigmoid table if none exists */
		RST.change_param(fc[track::midtone] / 100.0f, static_cast<float>(fc[track::strength]));

		if (fc.none_of(check::R, check::G, check::B))
		{
			/* Scan Y channel data */
			parallel::par_for(fpip->h, [fpip](int begin, int end) {
				for (int r = begin; r < end; r++)
				{
					for (int c = 0; c < fpip->w; c++)
					{
						PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
						px->y = RST.lookup(px->y);
					}
				}
			});
		}
		else //RGB mode
		{
			parallel::par_for(fpip->h, [fpip, fc](int begin, int end) {
				float buf[4] = { 0 };
				for (int r = begin; r < end; r++)
				{
					for (int c = 0; c < fpip->w; c++)
					{
						PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
						color_cvt::yc2rgb(buf, px);
						// transform each channel is needed
						//PIXEL t_rgb{ 0 };
						if (fc[check::R]){
							buf[3] = static_cast<float>(RST.lookup(buf[3]));
						}
						if (fc[check::G]){
							buf[2] = static_cast<float>(RST.lookup(buf[2]));
						}
						if (fc[check::B]){
							buf[1] = static_cast<float>(RST.lookup(buf[1]));
						}
						color_cvt::rgb2yc(px, buf);
					}
				}

			});
		}
#ifdef USECLOCK
		if (fc.any_of(check::echo_benchmark, check::save_benchmark)){
			const auto end_sd = ch::steady_clock::now();
			const auto elapsed = end_sd - start_sd;
			if (fc[check::echo_benchmark]) {
				using namespace std::chrono_literals;//UDLs : ms
				static auto last_echo_time = end_sd;
				if (last_echo_time == end_sd || last_echo_time + 150ms < end_sd) {
					const auto elapsed_s = std::to_string(ch::duration_cast<ch::microseconds>(elapsed).count());
					SetWindowText(fc.window_handle(), ("SDeCon:" + elapsed_s + "micro sec. @" + std::to_string(fpip->w) + "x" + std::to_string(fpip->h)).c_str());
					fc.notify_update_window();
					last_echo_time = end_sd;
				}
			}
			if (fc[check::save_benchmark]) {
				logbuf.push_back(ch::duration_cast<ch::nanoseconds>(elapsed).count());
			}
		}
#endif

		return TRUE; //TRUE to update frame image. FALSE to skip refresh.
	}
	// This is used for capturing mouse click, button states, and getting mouse coordinates
	BOOL WndProc(HWND /*hwnd*/, UINT message, WPARAM /*wparam*/, LPARAM /*lparam*/, void* /*editp*/, FILTER* fp)
	{
#ifdef USECLOCK
		switch (message)
		{
			case WM_FILTER_EXPORT:
			case WM_FILTER_SAVE_START:
				disable_echo_benchmark(fp);
				break;
				//case WM_FILTER_MAIN_MOUSE_DOWN:
				//case WM_FILTER_MAIN_MOUSE_MOVE:
				//case WM_FILTER_FILE_CLOSE:
			default:
				break;
		}
#endif
		return FALSE;
	}
	BOOL save_start(FILTER* fp, int /*s*/, int /*e*/, void* /*editp*/)
	{
#ifdef USECLOCK
		disable_echo_benchmark(fp);
#endif
		return TRUE;
	}

	BOOL save_end(FILTER* /*fp*/, void* /*editp*/)
	{
#ifdef USECLOCK
		if (!logbuf.empty()) {
			std::ofstream logfile_sd("log_sd.csv");
			for (auto&& i : logbuf) {
				logfile_sd << "SDeCon," << i << std::endl;
			}
		}
#endif
		return TRUE;
	}

	static FILTER_DLL filter_info_en = {               // English UI filter info
		FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_PRIORITY_LOWEST,	//	filter flags, use bitwise OR to add more
		0, 0,							//	dialogbox size
		PLUGIN_NAME_SDCON,				//	Filter plugin name
		TRACK_N,						//	トラックバーの数 (0なら名前初期値等もNULLでよい)
		en_name,						//	slider label names in English
		track_default,					//	トラックバーの初期値郡へのポインタ
		track_s, track_e,				//	トラックバーの数値の下限上限 (NULLなら全て0～256)
		CHECK_N,						//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
		check_name_en,					//	チェックボックスの名前郡へのポインタ
		check_default,					//	チェックボックスの初期値郡へのポインタ
		proc,		//	main filter function, use NULL to skip
		init,		//	initialization function, use NULL to skip
		exit,		//	on-exit function, use NULL to skip
		update,		//	invokes when when settings changed. use NULL to skip
		NULL,							//	for capturing dialog's control messages. Essential if you use button or auto uncheck some checkboxes.
		NULL, NULL,						//	Reserved. Do not use.
		NULL,							//  pointer to extra data when FILTER_FLAG_EX_DATA is set
		NULL,							//  extra data size
		VERSION_STR_SDCON,
		//  pointer or c-string for full filter info when FILTER_FLAG_EX_INFORMATION is set.
		save_start,	//	invoke just before saving starts. NULL to skip
		save_end,	//	invoke just after saving ends. NULL to skip
	};
}

// Export the above filter table
EXTERN_C  __declspec(dllexport) FILTER_DLL** GetFilterTableList(void)
{
	//must terminate with nullptr
	//http://qiita.com/yumetodo/items/4d972da03b3be788fcda
	static FILTER_DLL* pluginlist[] = { &sigmoid_contrast::filter_info_en, &sigmoid_decontrast::filter_info_en, nullptr };
	return pluginlist;
}

/**************************************************************************************/
/* IMPORTANT NOTE */
/* The original SDK sample code use the following:

EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void )
{
return &filter;
}

but MSVC seems to have problem parsing this and you will need an extra .def file
for proper export.
If the FILTER_DLL struct fails to export, the plugin will not show up in AviUtl's
"Filter+" menu.
*/
/**************************************************************************************/
