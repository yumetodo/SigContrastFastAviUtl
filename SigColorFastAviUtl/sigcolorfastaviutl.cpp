#define NOMINMAX
#include <Windows.h>
#include "filter.h" //please set this to AviUtl SDK's filter.h
#include "SigmoidTable.h"
#include "RSigmoidTable.h"
#include <thread>
#include <type_traits>
#include <vector>
#define USECLOCK
#ifdef USECLOCK
#include <chrono>
#include <ctime>
#include <ratio>
#include <string>
#endif


#ifdef _DEBUG
#define PLUGIN_NAME_SCON "SContrast DEBUG"
#define VERSION_STR_SCON "SContrast(DEBUG) v0.1 by MaverickTse"
#define PLUGIN_NAME_SDCON "SDeContrast DEBUG"
#define VERSION_STR_SDCON "SDeContrast(DEBUG) v0.1 by MaverickTse"
#else
#define PLUGIN_NAME_SCON "SContrast"
#define VERSION_STR_SCON "SContrast v0.1 by MaverickTse"
#define PLUGIN_NAME_SDCON "SDeContrast"
#define VERSION_STR_SDCON "SDeContrast v0.1 by MaverickTse"
#endif

bool prevIsYC_Con = true;
bool prevIsYC_SD = true;
// Define sliders
#define	TRACK_N	2 //	slider count. This creates 3 sliders
// i18n slider name
char* en_name[] = { "Midtone", "Strength" };
//char* jp_name[] = { "中間値", "強さ" };
//char* cht_name[] = { "中間值", "強度" };

int		track_default[] = { 50, 5 };	//	default values
int		track_s[] = { 0, 1 };	//	minimum values
int		track_e[] = { 100, 30 };	//	maximum values

											// Define checkboxes and buttons
#define	CHECK_N	5														//	total number of check box and button
char	*check_name_en[] = { "Y", "R", "G", "B", "Benchmark" };				//	label name
int		check_default[] = { 1, 0, 0, 0 };				//	for checkbox: 0(unchecked) or 1(checked); for button: must be -1

//char	*check_name_jp[] = { "Y", "R", "G", "B" };				//	label name:JP
//char	*check_name_cht[] = { "Y", "R", "G", "B" };				//	label name:CHT

#ifdef USECLOCK
std::chrono::time_point<std::chrono::steady_clock> start_con, end_con, start_sd, end_sd;
#endif

SigmoidTable* ST = nullptr;
RSigmoidTable* RST = nullptr;


													// Define filter info
FILTER_DLL scon_en = {               // English UI filter info
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_PRIORITY_LOWEST,	//	filter flags, use bitwise OR to add more
																//	FILTER_FLAG_ALWAYS_ACTIVE		: フィルタを常にアクティブにします
																//	FILTER_FLAG_CONFIG_POPUP		: 設定をポップアップメニューにします
																//	FILTER_FLAG_CONFIG_CHECK		: 設定をチェックボックスメニューにします
																//	FILTER_FLAG_CONFIG_RADIO		: Only one of the checkboxes can be ticked at one time.
																//	FILTER_FLAG_EX_DATA				: 拡張データを保存出来るようにします。
																//	FILTER_FLAG_PRIORITY_HIGHEST	: Make this plugin highest priority(i.e. always run before other filters)
																//	FILTER_FLAG_PRIORITY_LOWEST		: Make this plugin lowest priority
																//	FILTER_FLAG_WINDOW_THICKFRAME	: user-draggable dialog box
																//	FILTER_FLAG_WINDOW_SIZE			: Custom dialogbox size (size defined in argument 2 and 3)
																//	FILTER_FLAG_DISP_FILTER			: 表示フィルタにします
																//	FILTER_FLAG_EX_INFORMATION		: フィルタの拡張情報を設定できるようにします
																//	FILTER_FLAG_NO_CONFIG			: 設定ウィンドウを表示しないようにします
																//	FILTER_FLAG_AUDIO_FILTER		: オーディオフィルタにします
																//	FILTER_FLAG_RADIO_BUTTON		: チェックボックスをラジオボタンにします
																//	FILTER_FLAG_WINDOW_HSCROLL		: 水平スクロールバーを持つウィンドウを作ります
																//	FILTER_FLAG_WINDOW_VSCROLL		: 垂直スクロールバーを持つウィンドウを作ります
																//	FILTER_FLAG_IMPORT				: インポートメニューを作ります
																//	FILTER_FLAG_EXPORT				: エクスポートメニューを作ります
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
																NULL,						//	invoke just before saving starts. NULL to skip
																NULL,						//	invoke just after saving ends. NULL to skip
};

FILTER_DLL sdecon_en = {               // English UI filter info
	FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_PRIORITY_LOWEST,	//	filter flags, use bitwise OR to add more
																//	FILTER_FLAG_ALWAYS_ACTIVE		: フィルタを常にアクティブにします
																//	FILTER_FLAG_CONFIG_POPUP		: 設定をポップアップメニューにします
																//	FILTER_FLAG_CONFIG_CHECK		: 設定をチェックボックスメニューにします
																//	FILTER_FLAG_CONFIG_RADIO		: Only one of the checkboxes can be ticked at one time.
																//	FILTER_FLAG_EX_DATA				: 拡張データを保存出来るようにします。
																//	FILTER_FLAG_PRIORITY_HIGHEST	: Make this plugin highest priority(i.e. always run before other filters)
																//	FILTER_FLAG_PRIORITY_LOWEST		: Make this plugin lowest priority
																//	FILTER_FLAG_WINDOW_THICKFRAME	: user-draggable dialog box
																//	FILTER_FLAG_WINDOW_SIZE			: Custom dialogbox size (size defined in argument 2 and 3)
																//	FILTER_FLAG_DISP_FILTER			: 表示フィルタにします
																//	FILTER_FLAG_EX_INFORMATION		: フィルタの拡張情報を設定できるようにします
																//	FILTER_FLAG_NO_CONFIG			: 設定ウィンドウを表示しないようにします
																//	FILTER_FLAG_AUDIO_FILTER		: オーディオフィルタにします
																//	FILTER_FLAG_RADIO_BUTTON		: チェックボックスをラジオボタンにします
																//	FILTER_FLAG_WINDOW_HSCROLL		: 水平スクロールバーを持つウィンドウを作ります
																//	FILTER_FLAG_WINDOW_VSCROLL		: 垂直スクロールバーを持つウィンドウを作ります
																//	FILTER_FLAG_IMPORT				: インポートメニューを作ります
																//	FILTER_FLAG_EXPORT				: エクスポートメニューを作ります
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
																NULL,						//	invoke just before saving starts. NULL to skip
																NULL,						//	invoke just after saving ends. NULL to skip
};

FILTER_DLL* pluginlist[] = { &scon_en, &sdecon_en };
// Export the above filter table
EXTERN_C  __declspec(dllexport) FILTER_DLL**  GetFilterTableList(void)
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

namespace parallel {
	template<
		typename Index, typename Func, 
		typename RawIndexType = Index, 
		std::enable_if_t<std::is_unsigned<Index>::value && std::is_integral<RawIndexType>::value, std::nullptr_t> = nullptr
	>
	void par_for(Index num, Func&& f) {
		const unsigned int thread_num = std::thread::hardware_concurrency();
		if (thread_num < 2) {//thread 非対応
			f(static_cast<RawIndexType>(0), static_cast<RawIndexType>(num));
		}
		else {
			const auto task_num = num / thread_num;
			std::vector<std::thread> th;
			th.reserve(thread_num);
			for (unsigned int i = 0; i < thread_num; ++i) {
				th.emplace_back(
					std::forward<Func>(f),
					static_cast<RawIndexType>(i * task_num),
					static_cast<RawIndexType>(((i + 1) == thread_num) ? num : (i + 1) * task_num)
				);
			}
			for (auto&& t : th) t.join();
		}
	}
	template<typename Index, typename Func, std::enable_if_t<std::is_signed<Index>::value, std::nullptr_t> = nullptr>
	void par_for(Index num, Func&& f) {
		if (0 < num) par_for<std::uintmax_t, Func, Index, nullptr>(static_cast<std::uintmax_t>(num), std::forward<Func>(f));
	}
}
BOOL func_proc_con(FILTER *fp, FILTER_PROC_INFO *fpip) // This is the main image manipulation function
{

	/* Create a sigmoid table if none exists */
	/* Should only be called after closing and then opening a new file */
#ifdef USECLOCK
	if (fp->check[4]) start_con = std::chrono::steady_clock::now();
#endif

	if (!ST)
	{
		if (fp->check[1] || fp->check[2] || fp->check[3]) prevIsYC_Con = false;
		int scale = (prevIsYC_Con ? 4096 : 255);
		ST = new SigmoidTable(static_cast<float>(fp->track[0]/100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}

	if (prevIsYC_Con)
	{
		/* Scan Y channel data */
		parallel::par_for(fpip->h, [fpip](int begin, int end) {
			for (int r = begin; r < end; r++)
			{
				for (int c = 0; c < fpip->w; c++)
				{
					PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
					const short new_y = static_cast<short>(ST->lookup(px->y));
					px->y = new_y;
				}
			}
		});
	}
	else //RGB mode
	{
		parallel::par_for(fpip->h, [fpip, fp](int begin, int end) {
			PIXEL rgb;
			for (int r = begin; r < end; r++)
			{
				for (int c = 0; c < fpip->w; c++)
				{
					PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
					fp->exfunc->yc2rgb(&rgb, px, 1);
					// transform each channel is needed
					//PIXEL t_rgb{ 0 };
					if (fp->check[1])
					{
						rgb.r = static_cast<unsigned char>(ST->lookup(rgb.r));
						//rgb.r = t_rgb.r;
					}
					if (fp->check[2])
					{
						rgb.g = static_cast<unsigned char>(ST->lookup(rgb.g));
						//rgb.g = t_rgb.g;
					}
					if (fp->check[3])
					{
						rgb.b = static_cast<unsigned char>(ST->lookup(rgb.b));
						//rgb.b = t_rgb.b;
					}
					// convert back
					fp->exfunc->rgb2yc(px, &rgb, 1);
				}
			}
		});
	}
#ifdef USECLOCK
	if (fp->check[4])
	{
		end_con = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::duration<double>>( end_con - start_con);
		auto sec = elapsed.count() * 1000;
		auto timestr = std::to_string(std::round(sec));
		auto decimal = timestr.find('.');
		auto cleaned = timestr.substr(0, decimal);
		std::string msg = "SCon:" + cleaned + "ms @" + std::to_string(fpip->w) + "x" + std::to_string(fpip->h);
		SetWindowText(fp->hwnd, msg.c_str());
		fp->exfunc->filter_window_update(fp);
	}
#endif

	return TRUE; //TRUE to update frame image. FALSE to skip refresh.
}

BOOL func_exit_con(FILTER *fp)
{
	//DO NOT PUT MessageBox here, crash the application!
	//MessageBox(NULL, "func_exit invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
	if (ST)
	{
		delete ST;
		ST = nullptr;
	}
		
	return TRUE;
}
BOOL func_update_con(FILTER *fp, int status)
{
	//TODO
	switch (status)
	{
	case FILTER_UPDATE_STATUS_TRACK:
		//MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK", "DEMO", MB_OK | MB_ICONINFORMATION);
	{
		if (ST) delete ST;
		int scale = (prevIsYC_Con ? 4096 : 255);
		ST= new SigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}
		break;
	case FILTER_UPDATE_STATUS_TRACK + 1:
		//MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK+1", "DEMO", MB_OK | MB_ICONINFORMATION);
	{
		if (ST) delete ST;
		int scale = (prevIsYC_Con ? 4096 : 255);
		ST = new SigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}
		break;
//	case FILTER_UPDATE_STATUS_TRACK + 2:
//		MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK+2", "DEMO", MB_OK | MB_ICONINFORMATION);
//		break;
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
	//	MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_CHECK", "DEMO", MB_OK | MB_ICONINFORMATION);
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
	//	MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_CHECK+1", "DEMO", MB_OK | MB_ICONINFORMATION);
		break;
	case FILTER_UPDATE_STATUS_CHECK + 2: // no effect since our 3rd checkbox is a button.
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
	//	MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_CHECK+2", "DEMO", MB_OK | MB_ICONINFORMATION);
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
	case FILTER_UPDATE_STATUS_CHECK + 4:
	{
		if (fp->check[4] == 0)
		{
			SetWindowText(fp->hwnd, PLUGIN_NAME_SCON);
		}
	}
	break;
	//default:
		//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
	}
	fp->exfunc->filter_window_update(fp);
	bool nowYCmode = !(fp->check[1] || fp->check[2] || fp->check[3]);
	if (nowYCmode != prevIsYC_Con)
	{
		
		int scale = (nowYCmode ? 4096 : 255);
		if (ST) delete ST;
		ST = new SigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
		prevIsYC_Con = nowYCmode;
	}
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
	{
		if (ST)
		{
			delete ST;
			ST = nullptr;
		}
		break;
	}
	//case WM_COMMAND: // This is for capturing dialog control's message, i.e. button-click
	//	switch (wparam)
	//	{
	//	case MID_FILTER_BUTTON:
	//	{
	//		if (fp->check[0] == 1)
	//				{
	//					fp->check[1] = 0;
	//					fp->check[2] = 0;
	//					fp->check[3] = 0;
	//				}
	//				else
	//				{
	//					fp->check[1] = 1;
	//					fp->check[2] = 1;
	//					fp->check[3] = 1;
	//				}
	//	}break;
	//	case MID_FILTER_BUTTON + 2: // This ID is the COMBINED order of checkbox and button.
	//								// so MID_FILTER_BUTTON = checkbox1
	//								// MID_FILTER_BUTTON+1 = checkbox2
	//								// MID_FILTER_BUTTON+2 = button1
	//								// but since the checkboxes are not buttons, the first two MID_FILTER_BUTTON have no effect.
	//		MessageBoxExW(NULL, L"This should show some Chinese text\n你好嗎?", L"BUTTON-CLICK", MB_OK | MB_ICONINFORMATION, MAKELANGID(LANG_CHINESE_TRADITIONAL, SUBLANG_CHINESE_HONGKONG));
	//		break;
	//	default:
	//		return FALSE;

	//	}
	}
	return FALSE;
}
/**********************************************************/

BOOL func_proc_sd(FILTER *fp, FILTER_PROC_INFO *fpip) // This is the main image manipulation function
{
#ifdef USECLOCK
	if (fp->check[4])
	{
		start_sd = std::chrono::steady_clock::now();
	}
#endif

	/* Create a Reverse sigmoid table if none exists */
	if (!RST)
	{
		if (fp->check[1] || fp->check[2] || fp->check[3]) prevIsYC_SD = false;
		int scale = (prevIsYC_SD ? 4096 : 255);
		RST = new RSigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}

	if (prevIsYC_SD)
	{
		/* Scan Y channel data */
		parallel::par_for(fpip->h, [fpip](int begin, int end) {
			for (int r = begin; r < end; r++)
			{
				for (int c = 0; c < fpip->w; c++)
				{
					PIXEL_YC* const px = fpip->ycp_edit + r* fpip->max_w + c;
					const short new_y = static_cast<short>(RST->lookup(px->y));
					px->y = new_y;
				}
			}
		});
	}
	else //RGB mode
	{
		parallel::par_for(fpip->h, [fpip, fp](int begin, int end) {
			for (int r = begin; r < end; r++)
			{
				PIXEL rgb;
				for (int c = 0; c < fpip->w; c++)
				{
					PIXEL_YC* px = fpip->ycp_edit + r* fpip->max_w + c;
					fp->exfunc->yc2rgb(&rgb, px, 1);
					// transform each channel is needed
					//PIXEL t_rgb{ 0 };
					if (fp->check[1])
					{
						rgb.r = static_cast<unsigned char>(RST->lookup(rgb.r));
						//rgb.r = t_rgb.r;
					}
					if (fp->check[2])
					{
						rgb.g = static_cast<unsigned char>(RST->lookup(rgb.g));
						//rgb.g = t_rgb.g;
					}
					if (fp->check[3])
					{
						rgb.b = static_cast<unsigned char>(RST->lookup(rgb.b));
						//rgb.b = t_rgb.b;
					}
					// convert back
					fp->exfunc->rgb2yc(px, &rgb, 1);
				}
			}

		});
	}
#ifdef USECLOCK
	if (fp->check[4])
	{
		end_sd = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end_sd - start_sd);
		auto sec = elapsed.count()*1000.0;
		auto timestr = std::to_string(std::round(sec));
		auto decimal = timestr.find('.');
		auto cleaned = timestr.substr(0, decimal);
		std::string msg = "SDeCon:" + cleaned + "ms @" + std::to_string(fpip->w) + "x" + std::to_string(fpip->h);
		SetWindowText(fp->hwnd, msg.c_str());
		fp->exfunc->filter_window_update(fp);
	}
#endif

	return TRUE; //TRUE to update frame image. FALSE to skip refresh.
}

BOOL func_exit_sd(FILTER *fp)
{
	//DO NOT PUT MessageBox here, crash the application!
	//MessageBox(NULL, "func_exit invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
	if (RST)
	{
		delete RST;
		RST = nullptr;
	}
	return TRUE;
}
BOOL func_update_sd(FILTER *fp, int status)
{
	//TODO
	switch (status)
	{
	case FILTER_UPDATE_STATUS_TRACK:
		//MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK", "DEMO", MB_OK | MB_ICONINFORMATION);
	{
		if (RST) delete RST;
		int scale = (prevIsYC_SD ? 4096 : 255);
		RST = new RSigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}
	break;
	case FILTER_UPDATE_STATUS_TRACK + 1:
		//MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK+1", "DEMO", MB_OK | MB_ICONINFORMATION);
	{
		if (RST) delete RST;
		int scale = (prevIsYC_SD ? 4096 : 255);
		RST = new RSigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}
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
	//	MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_CHECK", "DEMO", MB_OK | MB_ICONINFORMATION);
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
	//	MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_CHECK+1", "DEMO", MB_OK | MB_ICONINFORMATION);
	break;
	case FILTER_UPDATE_STATUS_CHECK + 2: // no effect since our 3rd checkbox is a button.
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
	//	MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_CHECK+2", "DEMO", MB_OK | MB_ICONINFORMATION);
	break;
	case FILTER_UPDATE_STATUS_CHECK + 3: // no effect since our 3rd checkbox is a button.
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
	case FILTER_UPDATE_STATUS_CHECK + 4:
	{
		if (fp->check[4] == 0)
		{
			SetWindowText(fp->hwnd, PLUGIN_NAME_SDCON);
		}
	}
	break;
	//default:
	//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
	}
	fp->exfunc->filter_window_update(fp);
	bool nowYCmode = !(fp->check[1] || fp->check[2] || fp->check[3]);
	if (nowYCmode != prevIsYC_SD)
	{

		int scale = (nowYCmode ? 4096 : 255);
		if (RST) delete RST;
		RST = new RSigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
		prevIsYC_SD = nowYCmode;
	}
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
		if (RST) {
			delete RST;
			RST = nullptr;
		}
		break;
	}
	//case WM_COMMAND: // This is for capturing dialog control's message, i.e. button-click
	//	switch (wparam)
	//	{
	//	case MID_FILTER_BUTTON + 2: // This ID is the COMBINED order of checkbox and button.
	//								// so MID_FILTER_BUTTON = checkbox1
	//								// MID_FILTER_BUTTON+1 = checkbox2
	//								// MID_FILTER_BUTTON+2 = button1
	//								// but since the checkboxes are not buttons, the first two MID_FILTER_BUTTON have no effect.
	//		MessageBoxExW(NULL, L"This should show some Chinese text\n你好嗎?", L"BUTTON-CLICK", MB_OK | MB_ICONINFORMATION, MAKELANGID(LANG_CHINESE_TRADITIONAL, SUBLANG_CHINESE_HONGKONG));
	//		break;
	//	default:
	//		return FALSE;

	//	}
	}
	return FALSE;
}