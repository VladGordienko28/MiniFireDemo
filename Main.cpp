#include <stdint.h>

#include <Windows.h>
#include <windowsx.h>

// Global constants.
//---------------------------------------------------------------------------------------------------------------------
static constexpr int32_t WORKING_AREA_WIDTH		= 256;
static constexpr int32_t WORKING_AREA_HEIGHT	= 256;

// A color.
//---------------------------------------------------------------------------------------------------------------------
struct Color
{
	uint8_t B, G, R, A;
};

// Global variables.
//---------------------------------------------------------------------------------------------------------------------
HDC			GDc;
HWND		GHwnd;
int32_t		GWinWidth = 0, GWinHeight = 0;

Color		GFrameBuffer[ WORKING_AREA_WIDTH * WORKING_AREA_HEIGHT ];

// Demo global variables.
//---------------------------------------------------------------------------------------------------------------------
// ..

//---------------------------------------------------------------------------------------------------------------------
void OnInit()
{
	// ...
}

//---------------------------------------------------------------------------------------------------------------------
void OnMouseMove( int32_t Button, int32_t X, int32_t Y )
{
	// ...
}

//---------------------------------------------------------------------------------------------------------------------
void OnMouseClick( int32_t Button, int32_t X, int32_t Y )
{
	// ...
}

//---------------------------------------------------------------------------------------------------------------------
void OnRedraw()
{
	// Just for experiments!
	static uint8_t val = 0;
	val += 5;

	memset( GFrameBuffer, val, sizeof( GFrameBuffer ) );

	for ( int i = 10; i < 100; ++i )
	{
		GFrameBuffer[ i + 20 * WORKING_AREA_WIDTH ] = Color{ 0, 0, 255, 0 };
	}


	// Blit.
	BITMAPINFO	BufferInfo;
	memset( &BufferInfo, 0, sizeof( BITMAPINFO ) );
	BufferInfo.bmiHeader.biSize			= sizeof( BITMAPINFO );
	BufferInfo.bmiHeader.biPlanes		= 1;
	BufferInfo.bmiHeader.biBitCount		= 32;
	BufferInfo.bmiHeader.biCompression	= BI_RGB;
	BufferInfo.bmiHeader.biWidth		= WORKING_AREA_WIDTH;
	BufferInfo.bmiHeader.biHeight		= -WORKING_AREA_HEIGHT; // Top-down image.

	StretchDIBits( GDc, 0, 0, GWinWidth, GWinHeight, 0, 0, WORKING_AREA_WIDTH, WORKING_AREA_HEIGHT, GFrameBuffer, &BufferInfo, DIB_RGB_COLORS, SRCCOPY );

	// Draw debug text.
	TextOut( GDc, 10, 10, "Hello", 5 );

	SelectPen( GDc, GetStockObject( WHITE_PEN ) );
	SelectBrush( GDc, GetStockObject( WHITE_BRUSH ) );
	MoveToEx( GDc, 10, 50, nullptr );
	LineTo( GDc, 100, 100 );
}

//---------------------------------------------------------------------------------------------------------------------
void OnKeyDown( uint32_t Key )
{
	// ...
}

//---------------------------------------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
		case WM_MOUSEMOVE:
		{
			// Mouse move event.
			static int32_t OldX = 0, OldY = 0;

			int32_t X = GET_X_LPARAM( lParam );
			int32_t Y = GET_Y_LPARAM( lParam );

			int32_t Button =	( wParam == MK_LBUTTON ) ? 1 :
								( wParam == MK_RBUTTON ) ? 2 : 0;

			if ( ( X != OldX ) || ( Y != OldY ) )
			{
				OnMouseMove( Button, X, Y );

				OldX = X;
				OldY = Y;
			}
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			// Mouse button click event.
			int32_t X = GET_X_LPARAM( lParam );
			int32_t Y = GET_Y_LPARAM( lParam );

			int32_t Button =	( message == WM_LBUTTONDOWN ) ? 1 :
								( message == WM_RBUTTONDOWN ) ? 2 : 0;

			OnMouseClick( Button, X, Y );
			return 0;
		}
		case WM_KEYDOWN:
		{
			// Key down.
			OnKeyDown( uint32_t( wParam ) );
			return 0;
		}
		case WM_TIMER:
		{
			// Timer.
			OnRedraw();
			return 0;
		}
		case WM_SIZE:
		{
			// Resize.
			GWinWidth = LOWORD( lParam );
			GWinHeight = HIWORD( lParam );
			return 0;
		}
		case WM_CLOSE:
		{
			DestroyWindow( hwnd );
			return 0;
		}
		case WM_DESTROY:
		{
			PostQuitMessage( 0 );
			return 0;
		}
	}

	// Default event processing.
	return DefWindowProc( hwnd, message, wParam, lParam );
}

// An application entry point.
//---------------------------------------------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// Init window class.
	static const char*	WINDOW_CLASS_NAME	= "FireDemoWindow";
	
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof( WNDCLASSEX );
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= nullptr;
	wcex.hCursor		= LoadCursor( nullptr, IDC_ARROW );
	wcex.hbrBackground	= (HBRUSH)( COLOR_BACKGROUND + 1 );
	wcex.lpszMenuName	= nullptr;
	wcex.lpszClassName	= WINDOW_CLASS_NAME;
	wcex.hIconSm		= nullptr;

	// Register window class.
	if ( !RegisterClassEx( &wcex ) )
	{
		return 1;
	}

	// Create a window.
	GHwnd = CreateWindow
	(
		WINDOW_CLASS_NAME,
		"Fire Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		600, 600,
		nullptr,
		nullptr,
		hInstance,
		0
	);

	GDc = GetDC( GHwnd );

	// Show the window.
	ShowWindow( GHwnd, SW_NORMAL );
	UpdateWindow( GHwnd );

	// Start the timer.
	SetTimer( GHwnd, 0, 30, nullptr );

	// Prepare framework.
	OnInit();

	// Main application loop.
	while ( true )
	{
		MSG Msg;

		while ( PeekMessage( &Msg, 0, 0, 0, PM_REMOVE ) )
		{
			if ( Msg.message == WM_QUIT )
				goto ExitLoop;

			TranslateMessage( &Msg );
			DispatchMessage( &Msg );
		}	
	}

ExitLoop:
	return 0;
}