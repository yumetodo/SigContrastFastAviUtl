#define NOMINMAX
#include <Windows.h>
#include "filter.h" //please set this to AviUtl SDK's filter.h
#include "SigmoidTable.hpp"
#include "RSigmoidTable.hpp"
#include "thread.hpp"
#include "color_cvt.hpp"
#define USECLOCK
#ifdef USECLOCK
#include <chrono>
#include <ctime>
#include <ratio>
#include <string>
#include <fstream>
#include <deque>
namespace ch = std::chrono;
std::deque<ch::nanoseconds::rep> logbuf_sc;
std::deque<ch::nanoseconds::rep> logbuf_sd;
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
#if defined(_MSC_VER) && defined(_DEBUG)
	using namespace std::string_literals;
	const auto s = fp->name + "::disable_echo_benchmark fp->check[6]:"s + std::to_string(fp->check[6]) + '\n';
	OutputDebugStringA(s.c_str());
#endif
	if (fp->check[6]) {
		fp->check[4] = fp->check[5] = 0;//disable benchmark
		fp->exfunc->filter_window_update(fp);
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

SigmoidTable ST;
RSigmoidTable RST;


													// Define filter info
FILTER_DLL scon_en = {               // English UI filter info
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_PRIORITY_LOWEST,	//	filter flags, use bitwise OR to add more
																0, 0,						//	dialogbox size
																PLUGIN_NAME_SCON,			//	Filter plugin name
																TRACK_N,					//	トラックバーの数 (0なら名前初期値等もNULLでよい)
																en_name,					//	slider label names in English
																track_default,				//	トラックバーの初期値郡へのポインタ
																track_s, track_e,			//	トラックバーの数値の下限上限 (NULLなら全て0～256)
																CHECK_N,					//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
																check_name_en,					//	チェックボックスの名前郡へのポインタ
																check_default,				//	チェックボックスの初期値郡へのポインタ
																func_proc_con,					//	main filter function, use NULL to skip
																func_init_con,						//	initialization function, use NULL to skip
																func_exit_con,						//	on-exit function, use NULL to skip
																func_update_con,						//	invokes when when settings changed. use NULL to skip
																func_WndProc_con,						//	for capturing dialog's control messages. Essential if you use button or auto uncheck some checkboxes.
																NULL, NULL,					//	Reserved. Do not use.
																NULL,						//  pointer to extra data when FILTER_FLAG_EX_DATA is set
																NULL,						//  extra data size
																VERSION_STR_SCON,
																//  pointer or c-string for full filter info when FILTER_FLAG_EX_INFORMATION is set.
																func_save_start_con,						//	invoke just before saving starts. NULL to skip
																func_save_end_con,						//	invoke just after saving ends. NULL to skip
};

FILTER_DLL sdecon_en = {               // English UI filter info
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_PRIORITY_LOWEST,	//	filter flags, use bitwise OR to add more
																0, 0,						//	dialogbox size
																PLUGIN_NAME_SDCON,			//	Filter plugin name
																TRACK_N,					//	トラックバーの数 (0なら名前初期値等もNULLでよい)
																en_name,					//	slider label names in English
																track_default,				//	トラックバーの初期値郡へのポインタ
																track_s, track_e,			//	トラックバーの数値の下限上限 (NULLなら全て0～256)
																CHECK_N,					//	チェックボックスの数 (0なら名前初期値等もNULLでよい)
																check_name_en,					//	チェックボックスの名前郡へのポインタ
																check_default,				//	チェックボックスの初期値郡へのポインタ
																func_proc_sd,					//	main filter function, use NULL to skip
																func_init_sd,						//	initialization function, use NULL to skip
																func_exit_sd,						//	on-exit function, use NULL to skip
																func_update_sd,						//	invokes when when settings changed. use NULL to skip
																NULL,						//	for capturing dialog's control messages. Essential if you use button or auto uncheck some checkboxes.
																NULL, NULL,					//	Reserved. Do not use.
																NULL,						//  pointer to extra data when FILTER_FLAG_EX_DATA is set
																NULL,						//  extra data size
																VERSION_STR_SDCON,
																//  pointer or c-string for full filter info when FILTER_FLAG_EX_INFORMATION is set.
																func_save_start_sd,						//	invoke just before saving starts. NULL to skip
																func_save_end_sd,						//	invoke just after saving ends. NULL to skip
};

FILTER_DLL* pluginlist[] = { &scon_en, &sdecon_en, nullptr };
// Export the above filter table
EXTERN_C  __declspec(dllexport) FILTER_DLL** GetFilterTableList(void)
{
	
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

BOOL func_init_con(FILTER *fp)
{
	return TRUE;
}

BOOL func_init_sd(FILTER *fp)
{
	return TRUE;
}
BOOL func_proc_con(FILTER *fp, FILTER_PROC_INFO *fpip) // This is the main image manipulation function
{

	/* Create a sigmoid table if none exists */
	/* Should only be called after closing and then opening a new file */
#ifdef USECLOCK
	ch::time_point<ch::steady_clock> start_con;
	if (fp->check[4] || fp->check[5]) start_con = ch::steady_clock::now();
#endif

	ST.change_param(static_cast<float>(fp->track[0]/100.0f), static_cast<float>(fp->track[1]));

	if (!(fp->check[1] || fp->check[2] || fp->check[3]))
	{
		/* Scan Y channel data */
		parallel::par_for(fpip->h, [fpip](int begin, int end) {
			for (int r = begin; r < end; r++)
			{
				for (int c = 0; c < fpip->w; c++)
				{
					PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
					const short new_y = static_cast<short>(ST.lookup(px->y));
					px->y = new_y;
				}
			}
		});
	}
	else //RGB mode
	{
		parallel::par_for(fpip->h, [fpip, fp](int begin, int end) {
			float buf[4] = { 0 };
			for (int r = begin; r < end; r++)
			{
				for (int c = 0; c < fpip->w; c++)
				{
					PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
					color_cvt::yc2rgb(buf, px);
					// transform each channel is needed
					if (fp->check[3])
					{
						buf[1] = static_cast<float>(ST.lookup(buf[1]));
					}
					if (fp->check[2])
					{
						buf[2] = static_cast<float>(ST.lookup(buf[2]));
					}
					if (fp->check[1])
					{
						buf[3] = static_cast<float>(ST.lookup(buf[3]));
					}
					// convert back
					color_cvt::rgb2yc(px, buf);
				}
			}
		});
	}
#ifdef USECLOCK
	if (fp->check[4] || fp->check[5])
	{
		const auto end_con = ch::steady_clock::now();
		const auto elapsed = end_con - start_con;
		if (fp->check[4]) {
			using namespace std::chrono_literals;//UDLs : ms
			static auto last_echo_time = end_con;
			if (last_echo_time == end_con || last_echo_time + 150ms < end_con) {
				const auto elapsed_s = std::to_string(ch::duration_cast<ch::microseconds>(elapsed).count());
				SetWindowText(fp->hwnd, ("SCon:" + elapsed_s + "micro sec. @" + std::to_string(fpip->w) + "x" + std::to_string(fpip->h)).c_str());
				fp->exfunc->filter_window_update(fp);
				last_echo_time = end_con;
			}
		}
		if (fp->check[5]) {
			logbuf_sc.push_back(ch::duration_cast<ch::nanoseconds>(elapsed).count());
		}
	}
#endif

	return TRUE; //TRUE to update frame image. FALSE to skip refresh.
}

BOOL func_exit_con(FILTER *fp)
{
	//DO NOT PUT MessageBox here, crash the application!
	//MessageBox(NULL, "func_exit invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
#ifdef USECLOCK
	if (!logbuf_sc.empty()) {
		std::ofstream logfile_sc("log_sc.csv");
		for (auto&& i : logbuf_sc) {
			logfile_sc << "SCon," << i << std::endl;
		}
	}
#endif
	return TRUE;
}
BOOL func_update_con(FILTER *fp, int status)
{
	
	switch (status)
	{
	case FILTER_UPDATE_STATUS_TRACK:
	case FILTER_UPDATE_STATUS_TRACK + 1:
		ST.change_param(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]));
		break;
	case FILTER_UPDATE_STATUS_CHECK:
	{
		if (fp->check[0] == 1)
		{
			fp->check[1] = 0;
			fp->check[2] = 0;
			fp->check[3] = 0;
		}
		else
		{
			fp->check[1] = 1;
			fp->check[2] = 1;
			fp->check[3] = 1;
		}
	}break;
	case FILTER_UPDATE_STATUS_CHECK + 1:
	{
		if (fp->check[1] == 1)
		{
			fp->check[0] = 0;
		}
		if ((fp->check[1] + fp->check[2] + fp->check[3]) == 0)
		{
			fp->check[0] = 1;
		}
	}break;
	case FILTER_UPDATE_STATUS_CHECK + 2: 
	{
		if (fp->check[2] == 1)
		{
			fp->check[0] = 0;
		}
		if ((fp->check[1] + fp->check[2] + fp->check[3]) == 0)
		{
			fp->check[0] = 1;
		}
	}
		break;
	case FILTER_UPDATE_STATUS_CHECK + 3: 
	{
		if (fp->check[3] == 1)
		{
			fp->check[0] = 0;
		}
		if ((fp->check[1] + fp->check[2] + fp->check[3]) == 0)
		{
			fp->check[0] = 1;
		}
	}
	break;
#ifdef USECLOCK
	case FILTER_UPDATE_STATUS_CHECK + 4:
	{
		if(0 == fp->check[4]) SetWindowText(fp->hwnd, PLUGIN_NAME_SCON);
	}
	break;
#endif
	//default:
		//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
	}
	fp->exfunc->filter_window_update(fp);

	return TRUE;
}
BOOL func_WndProc_con(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp)
// This is used for capturing mouse click, button states, and getting mouse coordinates
{
	//
	switch (message)
	{
	case WM_FILTER_MAIN_MOUSE_DOWN:
		break;
	case WM_FILTER_MAIN_MOUSE_MOVE:
		break;
	case WM_FILTER_FILE_CLOSE:
		break;
#ifdef USECLOCK
	case WM_FILTER_EXPORT:
	case WM_FILTER_SAVE_START:
		disable_echo_benchmark(fp);
		break;
#endif
	}
	return FALSE;
}
/**********************************************************/

BOOL func_proc_sd(FILTER *fp, FILTER_PROC_INFO *fpip) // This is the main image manipulation function
{
#ifdef USECLOCK
	ch::time_point<ch::steady_clock> start_sd;
	if (fp->check[4] || fp->check[5]) start_sd = ch::steady_clock::now();
#endif

	/* Create a Reverse sigmoid table if none exists */
	RST.change_param(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]));

	if (!(fp->check[1] || fp->check[2] || fp->check[3]))
	{
		/* Scan Y channel data */
		parallel::par_for(fpip->h, [fpip](int begin, int end) {
			for (int r = begin; r < end; r++)
			{
				for (int c = 0; c < fpip->w; c++)
				{
					PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
					const short new_y = static_cast<short>(RST.lookup(px->y));
					px->y = new_y;
				}
			}
		});
	}
	else //RGB mode
	{
		parallel::par_for(fpip->h, [fpip, fp](int begin, int end) {
			float buf[4] = { 0 };
			for (int r = begin; r < end; r++)
			{
				for (int c = 0; c < fpip->w; c++)
				{
					PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
					color_cvt::yc2rgb(buf, px);
					// transform each channel is needed
					//PIXEL t_rgb{ 0 };
					if (fp->check[1])
					{
						buf[3] = static_cast<float>(RST.lookup(buf[3]));
					}
					if (fp->check[2])
					{
						buf[2] = static_cast<float>(RST.lookup(buf[2]));
					}
					if (fp->check[3])
					{
						buf[1] = static_cast<float>(RST.lookup(buf[1]));
					}
					// convert back
					color_cvt::rgb2yc(px, buf);
				}
			}

		});
	}
#ifdef USECLOCK
	if (fp->check[4] || fp->check[5])
	{
		const auto end_sd = ch::steady_clock::now();
		const auto elapsed = end_sd - start_sd;
		if (fp->check[4]) {
			using namespace std::chrono_literals;//UDLs : ms
			static auto last_echo_time = end_sd;
			if (last_echo_time == end_sd || last_echo_time + 150ms < end_sd) {
				const auto elapsed_s = std::to_string(ch::duration_cast<ch::microseconds>(elapsed).count());
				SetWindowText(fp->hwnd, ("SDeCon:" + elapsed_s + "micro sec. @" + std::to_string(fpip->w) + "x" + std::to_string(fpip->h)).c_str());
				fp->exfunc->filter_window_update(fp);
				last_echo_time = end_sd;
			}
		}
		if (fp->check[5]) {
			logbuf_sd.push_back(ch::duration_cast<ch::nanoseconds>(elapsed).count());
		}
	}
#endif

	return TRUE; //TRUE to update frame image. FALSE to skip refresh.
}

BOOL func_exit_sd(FILTER *fp)
{
	//DO NOT PUT MessageBox here, crash the application!
#ifdef USECLOCK
	if (!logbuf_sd.empty()) {
		std::ofstream logfile_sd("log_sd.csv");
		for (auto&& i : logbuf_sd) {
			logfile_sd << "SDeCon," << i << std::endl;
		}
	}
#endif
	return TRUE;
}
BOOL func_update_sd(FILTER *fp, int status)
{
	switch (status)
	{
	case FILTER_UPDATE_STATUS_TRACK:
	case FILTER_UPDATE_STATUS_TRACK + 1:
		RST.change_param(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]));
		break;
	case FILTER_UPDATE_STATUS_CHECK:
	{
		if (fp->check[0] == 1)
		{
			fp->check[1] = 0;
			fp->check[2] = 0;
			fp->check[3] = 0;
		}
		else
		{
			fp->check[1] = 1;
			fp->check[2] = 1;
			fp->check[3] = 1;
		}
	}
	break;
	case FILTER_UPDATE_STATUS_CHECK + 1:
	{
		if (fp->check[1] == 1)
		{
			fp->check[0] = 0;
		}
		if ((fp->check[1] + fp->check[2] + fp->check[3]) == 0)
		{
			fp->check[0] = 1;
		}
	}
	break;
	case FILTER_UPDATE_STATUS_CHECK + 2: 
	{
		if (fp->check[2] == 1)
		{
			fp->check[0] = 0;
		}
		if ((fp->check[1] + fp->check[2] + fp->check[3]) == 0)
		{
			fp->check[0] = 1;
		}
	}
	break;
	case FILTER_UPDATE_STATUS_CHECK + 3: 
	{
		if (fp->check[3] == 1)
		{
			fp->check[0] = 0;
		}
		if ((fp->check[1] + fp->check[2] + fp->check[3]) == 0)
		{
			fp->check[0] = 1;
		}
	}
	break;
#ifdef USECLOCK
	case FILTER_UPDATE_STATUS_CHECK + 4:
	{
		if (0 == fp->check[4]) SetWindowText(fp->hwnd, PLUGIN_NAME_SDCON);
	}
	break;
#endif
	}
	fp->exfunc->filter_window_update(fp);
	return TRUE;
}
BOOL func_WndProc_sd(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp)
// This is used for capturing mouse click, button states, and getting mouse coordinates
{
	//
	switch (message)
	{
	case WM_FILTER_MAIN_MOUSE_DOWN:
		break;
	case WM_FILTER_MAIN_MOUSE_MOVE:
		break;
	case WM_FILTER_FILE_CLOSE:
	{
		break;
	}
#ifdef USECLOCK
	case WM_FILTER_EXPORT:
	case WM_FILTER_SAVE_START:
		disable_echo_benchmark(fp);
		break;
#endif
	}
	return FALSE;
}

BOOL func_save_start_con(FILTER *fp, int s, int e, void *editp)
{
#ifdef USECLOCK
	disable_echo_benchmark(fp);
#endif
	return TRUE;
}
BOOL func_save_start_sd(FILTER *fp, int s, int e, void *editp)
{
#ifdef USECLOCK
	disable_echo_benchmark(fp);
#endif
	return TRUE;
}

BOOL func_save_end_con(FILTER *fp, void *editp)
{
#ifdef USECLOCK
	if (!logbuf_sc.empty()) {
		std::ofstream logfile_sc("log_sc.csv");
		for (auto&& i : logbuf_sc) {
			logfile_sc << "SCon," << i << std::endl;
		}
	}
#endif
	return TRUE;
}
BOOL func_save_end_sd(FILTER *fp, void *editp)
{
#ifdef USECLOCK
	if (!logbuf_sd.empty()) {
		std::ofstream logfile_sd("log_sd.csv");
		for (auto&& i : logbuf_sd) {
			logfile_sd << "SDeCon," << i << std::endl;
		}
	}
#endif
	return TRUE;
}
