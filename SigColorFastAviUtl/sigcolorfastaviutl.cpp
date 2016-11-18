#define NOMINMAX
#include <Windows.h>
#include "filter.h" //please set this to AviUtl SDK's filter.h
#include "version.h"
#include "SigmoidTable.hpp"
#include "RSigmoidTable.hpp"
#include "thread.hpp"
#include "color_cvt.hpp"
#include "filter_helper.hpp"
#ifdef USECLOCK
#include "time_logger.hpp"
#include "window_title_manager.hpp"
#include <chrono>
#include <ctime>
#include <ratio>
namespace ch = std::chrono;
#endif

static const char* plugin_version_str = SIG_COLOR_AUF_INFO " v" SIG_COLOR_VERSION_STR " by " SIG_COLOR_AUTHOR;

// Define sliders
// slider name
static const char* en_name[] =	{ "Midtone",	"Strength" };

static int	track_default[] =	{ 50,			5 };	//	default values
static int	track_s[] =			{ 0,			1 };	//	minimum values
static int	track_e[] =			{ 100,			30 };	//	maximum values
static_assert(
	std_future::size(en_name) == std_future::size(track_default)
	&& std_future::size(track_default) == std_future::size(track_s)
	&& std_future::size(track_s) == std_future::size(track_e)
	&& std_future::size(track_s) == static_cast<std::size_t>(track::size),
	"error"
);

static const char* check_name_en[] = {
	"Y", "R", "G", "B"
#ifdef USECLOCK
	, "Echo benchmark", "Save benchmark", "Disable benchmark during export"
#endif
};				//	label name
static int	check_default[] = {
	1, 0, 0, 0
#ifdef USECLOCK
	,1,0,1
#endif
};				//	for checkbox: 0(unchecked) or 1(checked); for button: must be -1
static_assert(
	std_future::size(check_name_en) == std_future::size(check_default)
	&& std_future::size(check_name_en) == static_cast<std::size_t>(check::size),
	"error"
);

namespace sigmoid_contrast {
	static SigmoidTable ST;
#ifdef _DEBUG
	static const char* plugin_name = "SContrast(DEBUG)";
#else
	static const char* plugin_name = "SContrast";
#endif
#ifdef USECLOCK
	time_logger logger(plugin_name, "log_sc.csv");
	window_title_manager window_title(plugin_name);
#endif
	BOOL init(FILTER* /*fp*/) noexcept { return TRUE; }
	BOOL exit(FILTER* fp)
	{
		//DO NOT PUT MessageBox here, crash the application!
#ifdef USECLOCK
		logger.write_out(fp);
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
				if (!fc[check::echo_benchmark]) window_title.print_default(fc);
				return TRUE;
#endif
			default:
				//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
				return TRUE;
		}
#ifdef USECLOCK
		logger.clear();
#endif
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
				for (int r = begin; r < end; r++){
					for (int c = 0; c < fpip->w; c++){
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
						if (fc[check::B]) buf[1] = static_cast<float>(ST.lookup(buf[1]));
						if (fc[check::G]) buf[2] = static_cast<float>(ST.lookup(buf[2]));
						if (fc[check::R]) buf[3] = static_cast<float>(ST.lookup(buf[3]));
						color_cvt::rgb2yc(px, buf);
					}
				}
			});
		}
#ifdef USECLOCK
		if (fc.any_of(check::echo_benchmark, check::save_benchmark)){
			const auto end_con = ch::steady_clock::now();
			const auto elapsed = end_con - start_con;
			if (fc[check::echo_benchmark]) window_title.notify_print_time(fc, fpip, end_con, elapsed);
			if (fc[check::save_benchmark]) logger.push(elapsed, fpip);
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
				filter_proxy(fp).disable_benchmark();
				logger.clear();
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
		filter_proxy(fp).disable_benchmark();
#endif
		return TRUE;
	}
	BOOL save_end(FILTER* fp, void* /*editp*/)
	{
#ifdef USECLOCK
		logger.write_out(fp);
#endif
		return TRUE;
	}

	static FILTER_DLL filter_info_en = {               // English UI filter info
		FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_PRIORITY_LOWEST,	//	filter flags, use bitwise OR to add more
		0, 0,							//	dialogbox size
		const_cast<char*>(plugin_name),				//	Filter plugin name
		static_cast<int>(std_future::size(en_name)),						//	トラックバーの数 (0なら名前初期値等もNULLでよい)
		const_cast<char**>(en_name),						//	slider label names in English
		track_default,					//	トラックバーの初期値郡へのポインタ
		track_s, track_e,				//	トラックバーの数値の下限上限 (NULLなら全て0～256)
		static_cast<int>(std_future::size(check_default)),						//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
		const_cast<char**>(check_name_en),					//	チェックボックスの名前郡へのポインタ
		check_default,					//	チェックボックスの初期値郡へのポインタ
		proc,			//	main filter function, use NULL to skip
		init,			//	initialization function, use NULL to skip
		exit,			//	on-exit function, use NULL to skip
		update,		//	invokes when when settings changed. use NULL to skip
		WndProc,		//	for capturing dialog's control messages. Essential if you use button or auto uncheck some checkboxes.
		nullptr, nullptr,						//	Reserved. Do not use.
		nullptr,							//  pointer to extra data when FILTER_FLAG_EX_DATA is set
		0,							//  extra data size
		const_cast<char*>(plugin_version_str),
		//  pointer or c-string for full filter info when FILTER_FLAG_EX_INFORMATION is set.
		save_start,	//	invoke just before saving starts. NULL to skip
		save_end,		//	invoke just after saving ends. NULL to skip
	};
}
namespace sigmoid_decontrast {
	static RSigmoidTable RST;
#ifdef _DEBUG
	static const char* plugin_name = "SDeContrast(DEBUG)";
#else
	static const char* plugin_name = "SDeContrast";
#endif
#ifdef USECLOCK
	time_logger logger(plugin_name, "log_sd.csv");
	window_title_manager window_title(plugin_name);
#endif

	BOOL init(FILTER* /*fp*/) noexcept { return TRUE; }
	BOOL exit(FILTER* fp)
	{
		//DO NOT PUT MessageBox here, crash the application!
#ifdef USECLOCK
		logger.write_out(fp);
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
				if (!fc[check::echo_benchmark]) window_title.print_default(fc);
				return TRUE;
#endif
			default:
				//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
				return TRUE;
		}
#ifdef USECLOCK
		logger.clear();
#endif
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

		if (fc.none_of(check::R, check::G, check::B)){
			/* Scan Y channel data */
			parallel::par_for(fpip->h, [fpip](int begin, int end) {
				for (int r = begin; r < end; r++){
					for (int c = 0; c < fpip->w; c++){
						PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
						px->y = RST.lookup(px->y);
					}
				}
			});
		}
		else{ //RGB mode
			parallel::par_for(fpip->h, [fpip, fc](int begin, int end) {
				float buf[4] = { 0 };
				for (int r = begin; r < end; r++){
					for (int c = 0; c < fpip->w; c++){
						PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
						color_cvt::yc2rgb(buf, px);
						// transform each channel is needed
						if (fc[check::R]) buf[3] = static_cast<float>(RST.lookup(buf[3]));
						if (fc[check::G]) buf[2] = static_cast<float>(RST.lookup(buf[2]));
						if (fc[check::B]) buf[1] = static_cast<float>(RST.lookup(buf[1]));
						color_cvt::rgb2yc(px, buf);
					}
				}
			});
		}
#ifdef USECLOCK
		if (fc.any_of(check::echo_benchmark, check::save_benchmark)){
			const auto end_sd = ch::steady_clock::now();
			const auto elapsed = end_sd - start_sd;
			if (fc[check::echo_benchmark]) window_title.notify_print_time(fc, fpip, end_sd, elapsed);
			if (fc[check::save_benchmark]) logger.push(elapsed, fpip);
		}
#endif

		return TRUE; //TRUE to update frame image. FALSE to skip refresh.
	}
	// This is used for capturing mouse click, button states, and getting mouse coordinates
	BOOL WndProc(HWND /*hwnd*/, UINT message, WPARAM /*wparam*/, LPARAM /*lparam*/, void* /*editp*/, FILTER* fp)
	{
#ifdef USECLOCK
		switch (message){
			case WM_FILTER_EXPORT:
			case WM_FILTER_SAVE_START:
				filter_proxy(fp).disable_benchmark();
				logger.clear();
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
		filter_proxy(fp).disable_benchmark();
#endif
		return TRUE;
	}

	BOOL save_end(FILTER* fp, void* /*editp*/)
	{
#ifdef USECLOCK
		logger.write_out(fp);
#endif
		return TRUE;
	}

	static FILTER_DLL filter_info_en = {               // English UI filter info
		FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_PRIORITY_LOWEST,	//	filter flags, use bitwise OR to add more
		0, 0,							//	dialogbox size
		const_cast<char*>(plugin_name),				//	Filter plugin name
		static_cast<int>(std_future::size(en_name)),						//	トラックバーの数 (0なら名前初期値等もNULLでよい)
		const_cast<char**>(en_name),						//	slider label names in English
		track_default,					//	トラックバーの初期値郡へのポインタ
		track_s, track_e,				//	トラックバーの数値の下限上限 (NULLなら全て0～256)
		static_cast<int>(std_future::size(check_default)),						//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
		const_cast<char**>(check_name_en),					//	チェックボックスの名前郡へのポインタ
		check_default,					//	チェックボックスの初期値郡へのポインタ
		proc,		//	main filter function, use NULL to skip
		init,		//	initialization function, use NULL to skip
		exit,		//	on-exit function, use NULL to skip
		update,		//	invokes when when settings changed. use NULL to skip
		WndProc,							//	for capturing dialog's control messages. Essential if you use button or auto uncheck some checkboxes.
		nullptr, nullptr,						//	Reserved. Do not use.
		nullptr,							//  pointer to extra data when FILTER_FLAG_EX_DATA is set
		0,							//  extra data size
		const_cast<char*>(plugin_version_str),
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
