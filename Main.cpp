#include <stdint.h>

#include <math.h>

#include <Windows.h>
#include <windowsx.h>

// Global constants.
//---------------------------------------------------------------------------------------------------------------------
static constexpr int32_t X_DIM	= 256;
static constexpr int32_t Y_DIM	= 256;

static constexpr int32_t X_MASK	= X_DIM - 1;
static constexpr int32_t Y_MASK = Y_DIM - 1;

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

Color		GFrameBuffer[ X_DIM * Y_DIM ];

// Demo global variables.
//---------------------------------------------------------------------------------------------------------------------
uint8_t GFireMap[ X_DIM * Y_DIM ];

uint8_t GHeatTable[ 1024 ];

Color GPalette[ 256 ];

// A naive random function.
//---------------------------------------------------------------------------------------------------------------------
uint8_t SpeedRand()
{
	return rand() & 0xff;
}

//---------------------------------------------------------------------------------------------------------------------
template< typename T >
inline T Clamp( const T& Value, const T& MinV, const T& MaxV )
{
	return Value < MinV ? MinV : Value > MaxV ? MaxV : Value;
}

//---------------------------------------------------------------------------------------------------------------------
enum class ESparkType : uint8_t
{
	Burn = 0,
	Sparkle,
	Blaze,

	BlazeHelper,
};

struct Spark
{
	uint8_t		X;
	uint8_t		Y;
	uint8_t		Heat;
	ESparkType	Type;

	uint8_t		VelX;
	uint8_t		VelY;
};

static constexpr uint32_t MAX_SPARKS = 1024;

Spark GSparks[ MAX_SPARKS ];
int32_t GNumSparks = 0;

void AddSpark( ESparkType Type, uint32_t X, uint32_t Y )
{
	if ( GNumSparks < MAX_SPARKS )
	{
		Spark& spark = GSparks[ GNumSparks++ ];

		spark.X		= X;
		spark.Y		= Y;
		spark.Heat	= 255;
		spark.Type	= Type;
	}
}

void DeleteSparks( uint32_t X, uint32_t Y )
{
	for ( int32_t i = 0; i < GNumSparks; ++i )
	{
		Spark& spark = GSparks[ i ];

		if ( (fabsf(spark.X - X) < 10) && (fabsf(spark.Y - Y) < 10) )
		{
			GSparks[ i ] = GSparks[ GNumSparks - 1 ];
			GNumSparks--;
		}
	}
}

void MoveSpark( Spark& spark )
{
	int32_t VelX = spark.VelX - 128;
	int32_t VelY = spark.VelY - 128;

	if ( VelX >= 0 )
    {
		if ( ( SpeedRand() & 0x7F ) < VelX )
			spark.X = ( ++spark.X ) & X_MASK;
	}
	else
	{
		if ( ( SpeedRand() & 0x7F ) < -VelX )
			spark.X = ( --spark.X ) & X_MASK;
	}

	if ( VelY >= 0 )
	{
		if ( ( SpeedRand() & 0x7F ) < VelY )
			spark.Y = ( ++spark.Y ) & Y_MASK;
	}
	else
	{
		if ( ( SpeedRand() & 0x7F ) < -VelY )
			spark.Y = ( --spark.Y ) & Y_MASK;
	}
}

void RedrawSparks()
{
	for ( int32_t i = 0; i < GNumSparks; ++i )
	{
		Spark& spark = GSparks[ i ];

		switch ( spark.Type )
		{
			case ESparkType::Burn:
			{
				GFireMap[ spark.Y * X_DIM + spark.X ] = SpeedRand();
				break;
			}
			case ESparkType::Sparkle:
			{
				uint32_t X = spark.X + ( ((int32_t)SpeedRand() * 64) >> 8 );
				GFireMap[ spark.Y * X_DIM + (X & X_MASK) ] = SpeedRand();
				break;
			}
			case ESparkType::Blaze:
			{
				if ( GNumSparks < MAX_SPARKS && SpeedRand() < 128 )
				{
					int32_t k = GNumSparks++;

					GSparks[ k ].Type = ESparkType::BlazeHelper;
					GSparks[ k ].X = spark.X;
					GSparks[ k ].Y = spark.Y;
					GSparks[ k ].Heat = 255;
					GSparks[ k ].VelX = SpeedRand();
					GSparks[ k ].VelY = SpeedRand();  
				}
				break;
			}
			case ESparkType::BlazeHelper:
			{
				spark.Heat -= 5;
				if ( spark.Heat >= 251 )
				{
					GNumSparks--;
					GSparks[ i ] = GSparks[ GNumSparks ];
					continue;
				}

				GFireMap[ spark.Y * X_DIM + spark.X ] = spark.Heat;

				MoveSpark( spark );
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void FireFilter1()
{
	for ( int32_t y = 0; y < Y_DIM; ++y )
	for ( int32_t x = 0; x < X_DIM; ++x )
	{
		const uint32_t A = GFireMap[ (y) * X_DIM + (x) ];
		const uint32_t B = GFireMap[ (y) * X_DIM + ((x + 1) & X_MASK) ];
		const uint32_t C = GFireMap[ (y) * X_DIM + ((x - 1) & X_MASK) ];
		const uint32_t D = GFireMap[ ((y + 1) & Y_MASK) * X_DIM + (x) ];

		GFireMap[ y * X_DIM + x ] = GHeatTable[ A + B + C + D ];
	}
}

//---------------------------------------------------------------------------------------------------------------------
void FireFilter2()
{
	for ( int32_t y = 0; y < Y_DIM; ++y )
	for ( int32_t x = 0; x < X_DIM; ++x )
	{
		const uint32_t A = GFireMap[ ((y + 1) & Y_MASK) * X_DIM + (x) ];
		const uint32_t B = GFireMap[ ((y + 1) & Y_MASK) * X_DIM + ((x + 1) & X_MASK) ];
		const uint32_t C = GFireMap[ ((y + 1) & Y_MASK) * X_DIM + ((x - 1) & X_MASK) ];
		const uint32_t D = GFireMap[ ((y + 2) & Y_MASK) * X_DIM + (x) ];

		GFireMap[ y * X_DIM + x ] = GHeatTable[ A + B + C + D ];
	}
}

//---------------------------------------------------------------------------------------------------------------------
void GenerateHeatTable( uint8_t Conductivity )
{
	for ( int32_t i = 0; i < 1024; ++i )
	{
		float t = float(i) / 1023;

		t = t - 0.005f;

		// ToDo: Better formula required.
		GHeatTable[ i ]	= Clamp( t, 0.f, 1.f ) * 255; 
	}
}


//---------------------------------------------------------------------------------------------------------------------
void OnInit()
{
	// Generate palette.
	for ( int32_t i = 0; i < 256; ++i )
	{
		const float t = float(i) / 255.f;

		GPalette[ i ].R	= uint8_t(Clamp( powf( t * 3.9f, 0.5f ), 0.f, 1.f ) * 255);
		GPalette[ i ].G	= uint8_t(Clamp( powf( t * 2.2f, 1.5f ), 0.f, 1.f ) * 255);
		GPalette[ i ].B	= uint8_t(Clamp( powf( t * 1.6f, 2.f ), 0.f, 1.f ) * 255);
		GPalette[ i ].A = 255;
	}

	// Conductivity table.
	GenerateHeatTable( 220 );

	// Initialize fire map.
	memset( GFireMap, 0, sizeof( GFireMap ) );




	// ...
}

//---------------------------------------------------------------------------------------------------------------------
void OnMouseMove( int32_t Button, int32_t X, int32_t Y )
{
	GFireMap[ Y * X_DIM + X ] = 255;


	if ( Button == 1 )
	{
		AddSpark( ESparkType::Blaze, X, Y );
	}
	if ( Button == 2 )
	{
		DeleteSparks( X, Y );
	}
}

//---------------------------------------------------------------------------------------------------------------------
void OnMouseClick( int32_t Button, int32_t X, int32_t Y )
{
	if ( Button == 1 )
	{
		AddSpark( ESparkType::Blaze, X, Y );
	}
}

//---------------------------------------------------------------------------------------------------------------------
void OnRedraw()
{

	// Set on fire :)
	//for ( int32_t i = 0; i < 20; ++i )
	//	GFireMap[ 100 * X_DIM + 100 + i ] = SpeedRand();


	RedrawSparks();

	FireFilter1();

	// Convert fire map to image.
	for ( int32_t y = 0; y < Y_DIM; ++y )
	for ( int32_t x = 0; x < X_DIM; ++x )
	{
		GFrameBuffer[ y * X_DIM + x ] = GPalette[ GFireMap[ y * X_DIM + x ] ];
	}

#if 0
	// Just for experiments!
	static uint8_t val = 0;
	val += 5;

	memset( GFrameBuffer, val, sizeof( GFrameBuffer ) );

	for ( int i = 10; i < 100; ++i )
	{
		GFrameBuffer[ i + 20 * X_DIM ] = Color{ 0, 0, 255, 0 };
	}
#endif

	// Blit.
	BITMAPINFO	BufferInfo;
	memset( &BufferInfo, 0, sizeof( BITMAPINFO ) );
	BufferInfo.bmiHeader.biSize			= sizeof( BITMAPINFO );
	BufferInfo.bmiHeader.biPlanes		= 1;
	BufferInfo.bmiHeader.biBitCount		= 32;
	BufferInfo.bmiHeader.biCompression	= BI_RGB;
	BufferInfo.bmiHeader.biWidth		= X_DIM;
	BufferInfo.bmiHeader.biHeight		= -Y_DIM; // Top-down image.

	StretchDIBits( GDc, 0, 0, GWinWidth, GWinHeight, 0, 0, X_DIM, Y_DIM, GFrameBuffer, &BufferInfo, DIB_RGB_COLORS, SRCCOPY );

	// Draw debug text.
	//TextOut( GDc, 10, 10, "Hello", 5 );

	//SelectPen( GDc, GetStockObject( WHITE_PEN ) );
	//SelectBrush( GDc, GetStockObject( WHITE_BRUSH ) );
	//MoveToEx( GDc, 10, 50, nullptr );
	//LineTo( GDc, 100, 100 );
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
				OnMouseMove( Button, int32_t(X_DIM*float(X)/float(GWinWidth)), int32_t(Y_DIM*float(Y)/float(GWinHeight)) );

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

			OnMouseClick( Button, int32_t(X_DIM*float(X)/float(GWinWidth)), int32_t(Y_DIM*float(Y)/float(GWinHeight)) );
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