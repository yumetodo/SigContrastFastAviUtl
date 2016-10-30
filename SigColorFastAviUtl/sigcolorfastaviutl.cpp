#define NOMINMAX
#include <Windows.h>
#include "filter.h" //please set this to AviUtl SDK's filter.h
#include "SigmoidTable.h"
#include "RSigmoidTable.h"
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
#define VERSION_STR_SCON "SContrast(DEBUG) v0.2 by MaverickTse"
#define PLUGIN_NAME_SDCON "SDeContrast DEBUG"
#define VERSION_STR_SDCON "SDeContrast(DEBUG) v0.2 by MaverickTse"
#else
#define PLUGIN_NAME_SCON "SContrast"
#define VERSION_STR_SCON "SContrast v0.2 by MaverickTse"
#define PLUGIN_NAME_SDCON "SDeContrast"
#define VERSION_STR_SDCON "SDeContrast v0.2 by MaverickTse"
#endif

#define YSCALE 4096
#define RGBSCALE 4096

#define COEFY 1.0037736040867458, 1.0031713814217937, 1.0038646965904563
#define COEFU 0.0009812686948862392, -0.34182057237626395, 1.7738420513779833
#define COEFV 1.4028706125758748, -0.7126004638855613, 0.0018494308641594699

#define COEFR 0.297607421875, -0.1689453125, 0.5
#define COEFG 0.586181640625, -0.331298828125, -0.419189453125
#define COEFB 0.11279296875, 0.5, -0.0810546875


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
#ifdef USECLOCK
#	define	CHECK_N	5														//	total number of check box and button
#else
#	define	CHECK_N	4														//	total number of check box and button
#endif
char	*check_name_en[] = { "Y", "R", "G", "B"
#ifdef USECLOCK
, "Benchmark"
#endif
};				//	label name
int		check_default[] = { 1, 0, 0, 0 };				//	for checkbox: 0(unchecked) or 1(checked); for button: must be -1

//char	*check_name_jp[] = { "Y", "R", "G", "B" };				//	label name:JP
//char	*check_name_cht[] = { "Y", "R", "G", "B" };				//	label name:CHT

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
																func_save_start_con,						//	invoke just before saving starts. NULL to skip
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
																func_save_start_sd,						//	invoke just before saving starts. NULL to skip
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


BOOL func_proc_con(FILTER *fp, FILTER_PROC_INFO *fpip) // This is the main image manipulation function
{

	/* Create a sigmoid table if none exists */
	/* Should only be called after closing and then opening a new file */
#ifdef USECLOCK
	ch::time_point<ch::steady_clock> start_con;
	if (fp->check[4]) start_con = ch::steady_clock::now();
#endif

	if (!ST)
	{
		if (fp->check[1] || fp->check[2] || fp->check[3]) prevIsYC_Con = false;
		int scale = (prevIsYC_Con ? YSCALE : RGBSCALE);
		ST = new SigmoidTable(static_cast<float>(fp->track[0]/100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}

	if (!(fp->check[1] || fp->check[2] || fp->check[3]))
	{
		/* Scan Y channel data */
		const int fh = fpip->h;

#pragma loop( hint_parallel(0) )
#pragma loop( ivdep )
		for (int r = 0; r < fh; r++)
		{
			for (int c = 0; c < fpip->w; c++)
			{
				PIXEL_YC* px = fpip->ycp_edit + r* fpip->max_w + c;
				short new_y = static_cast<short>(ST->lookup(px->y));
				px->y = new_y;
			}
		}
	}
	else //RGB mode
	{
		const int fh = fpip->h;
		const int fw = fpip->w;
		// Load YUV2RGB matrix
		__m128 cy = _mm_set_ps(COEFY, 0.f);
		__m128 cu = _mm_set_ps(COEFU, 0.f);
		__m128 cv = _mm_set_ps(COEFV, 0.f);
		// Load RGB2YUV matrix
		__m128 cr = _mm_set_ps(COEFR, 0.f);
		__m128 cg = _mm_set_ps(COEFG, 0.f);
		__m128 cb = _mm_set_ps(COEFB, 0.f);
#pragma loop( hint_parallel(0) )
#pragma loop( ivdep )
		for (int r = 0; r < fh; r++)
		{
			//PIXEL* rgb = new PIXEL;
			float buf[4] = { 0 };
#pragma loop( no_vector )
			for (int c = 0; c < fw; c++)
			{
				PIXEL_YC* px = fpip->ycp_edit + r* fpip->max_w + c;
				//fp->exfunc->yc2rgb(rgb, px, 1);
				//implement our conversion
				__m128 my = _mm_set1_ps(static_cast<float>(px->y));
				__m128 mu = _mm_set1_ps(static_cast<float>(px->cb));
				__m128 mv = _mm_set1_ps(static_cast<float>(px->cr));

				my = _mm_mul_ps(my, cy);
				mu = _mm_mul_ps(mu, cu);
				mv = _mm_mul_ps(mv, cv);

				my = _mm_add_ps(my, mu);
				my = _mm_add_ps(my, mv); //result in my
				
				_mm_storeu_ps(buf, my); // buf: 0, b, g, r
				// End of YUV2RGB intrinsics

				// transform each channel is needed
				//PIXEL t_rgb{ 0 };
				if (fp->check[3])
				{
					//rgb->b = static_cast<unsigned char>(ST->lookup(rgb->b));
					buf[1] = static_cast<float>(ST->lookup(static_cast<int>(buf[1])));
					//rgb.b = t_rgb.b;
				}
				if (fp->check[2])
				{
					//rgb->g = static_cast<unsigned char>(ST->lookup(rgb->g));
					buf[2] = static_cast<float>(ST->lookup(static_cast<int>(buf[2])));
					//rgb.g = t_rgb.g;
				}
				if (fp->check[1])
				{
					//rgb->r = static_cast<unsigned char>(ST->lookup(rgb->r));
					buf[3] = static_cast<float>(ST->lookup(static_cast<int>(buf[3])));
					//rgb.r = t_rgb.r;
				}
				
				
				// convert back
				//fp->exfunc->rgb2yc(px, rgb, 1);
				my = _mm_set1_ps(buf[1]);
				mu = _mm_set1_ps(buf[2]);
				mv = _mm_set1_ps(buf[3]);
				my = _mm_mul_ps(my, cb);
				mu = _mm_mul_ps(mu, cg);
				mv = _mm_mul_ps(mv, cr);
				my = _mm_add_ps(my, mu);
				my = _mm_add_ps(my, mv); //result in my
				
				_mm_storeu_ps(buf, my); // buf: 0, v, u, y
				px->y = static_cast<short>(buf[3]);
				px->cb = static_cast<short>(buf[2]);
				px->cr = static_cast<short>(buf[1]);
			}
			//delete rgb;
		}
	}
#ifdef USECLOCK
	if (fp->check[4])
	{
		const auto end_con = ch::steady_clock::now();
		logbuf_sc.push_back(ch::duration_cast<ch::nanoseconds>(end_con - start_con).count());
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
	//TODO
	switch (status)
	{
	case FILTER_UPDATE_STATUS_TRACK:
		//MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK", "DEMO", MB_OK | MB_ICONINFORMATION);
	{
		if (ST) delete ST;
		//int scale = (prevIsYC_Con ? YSCALE : RGBSCALE);
		int scale = YSCALE;
		ST= new SigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}
		break;
	case FILTER_UPDATE_STATUS_TRACK + 1:
		//MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK+1", "DEMO", MB_OK | MB_ICONINFORMATION);
	{
		if (ST) delete ST;
		//int scale = (prevIsYC_Con ? YSCALE : RGBSCALE);
		int scale = YSCALE;
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
		SetWindowText(fp->hwnd, PLUGIN_NAME_SCON);
	}
	break;
	//default:
		//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
	}
	fp->exfunc->filter_window_update(fp);
	//bool nowYCmode = !(fp->check[1] || fp->check[2] || fp->check[3]);
	//if (nowYCmode != prevIsYC_Con)
	//{
	//	
	//	int scale = (nowYCmode ? YSCALE : RGBSCALE);
	//	if (ST) delete ST;
	//	ST = new SigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	//	prevIsYC_Con = nowYCmode;
	//}
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
	case WM_FILTER_EXPORT:
	{
		fp->check[4] = 0;
		fp->exfunc->filter_window_update(fp);
	}break;
	case WM_FILTER_SAVE_START:
	{
		fp->check[4] = 0;
		fp->exfunc->filter_window_update(fp);
	}break;
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
	ch::time_point<ch::steady_clock> start_sd;
	if (fp->check[4]) start_sd = ch::steady_clock::now();
#endif

	/* Create a Reverse sigmoid table if none exists */
	if (!RST)
	{
		if (fp->check[1] || fp->check[2] || fp->check[3]) prevIsYC_SD = false;
		//int scale = (prevIsYC_SD ? YSCALE : RGBSCALE);
		int scale = YSCALE;
		RST = new RSigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}

	if (!(fp->check[1] || fp->check[2] || fp->check[3]))
	{
		/* Scan Y channel data */
		const int fh = fpip->h;

#pragma loop( hint_parallel(0) )
#pragma loop( ivdep )
		for (int r = 0; r < fh; r++)
		{
			for (int c = 0; c < fpip->w; c++)
			{
				PIXEL_YC* px = fpip->ycp_edit + r* fpip->max_w + c;
				short new_y = static_cast<short>(RST->lookup(px->y));
				px->y = new_y;
			}
		}
	}
	else //RGB mode
	{
		const int fh = fpip->h;
		const int fw = fpip->w;
		// Load YUV2RGB matrix
		__m128 cy = _mm_set_ps(COEFY, 0.f);
		__m128 cu = _mm_set_ps(COEFU, 0.f);
		__m128 cv = _mm_set_ps(COEFV, 0.f);
		// Load RGB2YUV matrix
		__m128 cr = _mm_set_ps(COEFR, 0.f);
		__m128 cg = _mm_set_ps(COEFG, 0.f);
		__m128 cb = _mm_set_ps(COEFB, 0.f);
#pragma loop( hint_parallel(0) )
#pragma loop( ivdep )
		for (int r = 0; r < fh; r++)
		{
			//PIXEL* rgb = new PIXEL;
			float buf[4] = { 0 };
#pragma loop( no_vector )
			for (int c = 0; c < fw; c++)
			{
				PIXEL_YC* px = fpip->ycp_edit + r* fpip->max_w + c;
				//fp->exfunc->yc2rgb(rgb, px, 1);
				//implement our conversion
				__m128 my = _mm_set1_ps(static_cast<float>(px->y));
				__m128 mu = _mm_set1_ps(static_cast<float>(px->cb));
				__m128 mv = _mm_set1_ps(static_cast<float>(px->cr));

				my = _mm_mul_ps(my, cy);
				mu = _mm_mul_ps(mu, cu);
				mv = _mm_mul_ps(mv, cv);

				my = _mm_add_ps(my, mu);
				my = _mm_add_ps(my, mv); //result in my

				_mm_storeu_ps(buf, my); // buf: 0, b, g, r
										// End of YUV2RGB intrinsics
				// transform each channel is needed
				//PIXEL t_rgb{ 0 };
				
				if (fp->check[1])
				{
					//rgb->r = static_cast<unsigned char>(ST->lookup(rgb->r));
					buf[3] = static_cast<float>(RST->lookup(static_cast<int>(buf[3])));
					//rgb.r = t_rgb.r;
				}
				if (fp->check[2])
				{
					//rgb->g = static_cast<unsigned char>(ST->lookup(rgb->g));
					buf[2] = static_cast<float>(RST->lookup(static_cast<int>(buf[2])));
					//rgb.g = t_rgb.g;
				}
				if (fp->check[3])
				{
					//rgb->b = static_cast<unsigned char>(ST->lookup(rgb->b));
					buf[1] = static_cast<float>(RST->lookup(static_cast<int>(buf[1])));
					//rgb.b = t_rgb.b;
				}
				// convert back
				//fp->exfunc->rgb2yc(px, rgb, 1);
				// convert back
				//fp->exfunc->rgb2yc(px, rgb, 1);
				my = _mm_set1_ps(buf[1]);
				mu = _mm_set1_ps(buf[2]);
				mv = _mm_set1_ps(buf[3]);
				my = _mm_mul_ps(my, cb);
				mu = _mm_mul_ps(mu, cg);
				mv = _mm_mul_ps(mv, cr);
				my = _mm_add_ps(my, mu);
				my = _mm_add_ps(my, mv); //result in my

				_mm_storeu_ps(buf, my); // buf: 0, v, u, y
				px->y = static_cast<short>(buf[3]);
				px->cb = static_cast<short>(buf[2]);
				px->cr = static_cast<short>(buf[1]);
			}
			//delete rgb;
			
		}
	}
#ifdef USECLOCK
	if (fp->check[4])
	{
		const auto end_sd = ch::steady_clock::now();
		logbuf_sd.push_back(ch::duration_cast<ch::nanoseconds>(end_sd - start_sd).count());
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
	//TODO
	switch (status)
	{
	case FILTER_UPDATE_STATUS_TRACK:
		//MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK", "DEMO", MB_OK | MB_ICONINFORMATION);
	{
		if (RST) delete RST;
		//int scale = (prevIsYC_SD ? YSCALE : RGBSCALE);
		int scale = YSCALE;
		RST = new RSigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
	}
	break;
	case FILTER_UPDATE_STATUS_TRACK + 1:
		//MessageBox(NULL, "func_update FILTER_UPDATE_STATUS_TRACK+1", "DEMO", MB_OK | MB_ICONINFORMATION);
	{
		if (RST) delete RST;
		//int scale = (prevIsYC_SD ? YSCALE : RGBSCALE);
		int scale = YSCALE;
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
		SetWindowText(fp->hwnd, PLUGIN_NAME_SDCON);
	}
	break;
	//default:
	//MessageBox(NULL, "func_update invoked!", "DEMO", MB_OK | MB_ICONINFORMATION);
	}
	fp->exfunc->filter_window_update(fp);
	/*bool nowYCmode = !(fp->check[1] || fp->check[2] || fp->check[3]);
	if (nowYCmode != prevIsYC_SD)
	{

		int scale = (nowYCmode ? YSCALE : RGBSCALE);
		if (RST) delete RST;
		RST = new RSigmoidTable(static_cast<float>(fp->track[0] / 100.0f), static_cast<float>(fp->track[1]), scale, static_cast<float>(scale));
		prevIsYC_SD = nowYCmode;
	}*/
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
		/*if (RST) {
			delete RST;
			RST = nullptr;
		}*/
		break;
	}
	case WM_FILTER_EXPORT:
	{
		fp->check[4] = 0;
		fp->exfunc->filter_window_update(fp);
	}break;
	case WM_FILTER_SAVE_START:
	{
		fp->check[4] = 0;
		fp->exfunc->filter_window_update(fp);
	}break;
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

BOOL func_save_start_con(FILTER *fp, int s, int e, void *editp)
{
	fp->check[4] = 0;
	return TRUE;
}
BOOL func_save_start_sd(FILTER *fp, int s, int e, void *editp)
{
	fp->check[4] = 0;
	return TRUE;
}