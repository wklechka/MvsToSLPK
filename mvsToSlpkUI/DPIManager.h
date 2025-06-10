#pragma once

class DPIManager
{
public:
	DPIManager() {
		init();
	};

	// Get screen DPI.
	int getDPIX() { return _dpiX; };
	int getDPIY() { return _dpiY; };

	// Convert between raw pixels and relative pixels.
	int scaleX(int x) { return MulDiv(x, _dpiX, 96); };
	int scaleY(int y) { return MulDiv(y, _dpiY, 96); };
	int unscaleX(int x) { return MulDiv(x, 96, _dpiX); };
	int unscaleY(int y) { return MulDiv(y, 96, _dpiY); };

private:
	int _dpiX = 0;
	int _dpiY = 0;

	void init() {
		HDC hdc = GetDC(NULL);
		if (hdc)
		{			
			_dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
			_dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
			ReleaseDC(NULL, hdc);
		}
	};
};
