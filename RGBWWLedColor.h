/**
 * RGBWWLed - simple Library for controlling RGB WarmWhite ColdWhite LEDs via PWM
 * @file
 * @author  Patrick Jahns http://github.com/patrickjahns
 *
 * All files of this project are provided under the LGPL v3 license.
 */
#pragma once

#include <Wiring/WConstants.h>
#include "RGBWWconst.h"

enum RGBWW_COLORMODE {
    RGB = 0, RGBWW = 1, RGBCW = 2, RGBWWCW = 3, NUM_COLORMODES = 4
};

enum RGBWW_HSVMODEL {
    RAW = 0, SPEKTRUM = 1, RAINBOW = 2, NUM_HSVMODELS = 3
};

enum RGBWW_CHANNELS {
    RED = 0, GREEN = 1, BLUE = 2, WW = 3, CW = 4, NUM_CHANNELS = 5
};

//struct for RGBW + Kelvin
struct RGBWCT {

    union {
        int r;
        int red;
    };
    union {
        int g;
        int green;
    };
    union {
        int b;
        int blue;
    };
    union {
        int w;
        int white;
    };
    union {
        int ct;
        int colortemp;
    };

    RGBWCT() {
    }
    RGBWCT(int red, int green, int blue, int white) :
            r(red), g(green), b(blue), w(white), ct(0) {
    }
    RGBWCT(int red, int green, int blue, int white, int kelvin) :
            r(red), g(green), b(blue), w(white), ct(kelvin) {
    }
    RGBWCT(const RGBWCT& rgbwct) {
        this->r = rgbwct.r;
        this->g = rgbwct.g;
        this->b = rgbwct.b;
        this->w = rgbwct.w;
        this->ct = rgbwct.ct;
    }

    RGBWCT& operator=(const RGBWCT& rgbwct) {
        this->r = rgbwct.r;
        this->g = rgbwct.g;
        this->b = rgbwct.b;
        this->w = rgbwct.w;
        this->ct = rgbwct.ct;
        return *this;
    }
};

struct ChannelOutput {
    union {
        int r;
        int red;
    };
    union {
        int g;
        int green;
    };
    union {
        int b;
        int blue;
    };
    union {
        int ww;
        int warmwhite;
    };
    union {
        int cw;
        int coldwhite;
    };

    ChannelOutput() {
    }

    ChannelOutput(int red, int green, int blue, int warmwhite, int coldwhite) :
            r(red), g(green), b(blue), ww(warmwhite), cw(coldwhite) {
    }
    ChannelOutput(const ChannelOutput& output) {
        this->r = output.r;
        this->g = output.g;
        this->b = output.b;
        this->ww = output.ww;
        this->cw = output.cw;
    }

    bool operator==(const ChannelOutput& output) const {
        return r == output.r && g == output.g && b == output.b && ww == output.ww && cw == output.cw;
    }

    bool operator!=(const ChannelOutput& obj) const {
        return !(*this == obj);
    }

    ChannelOutput& operator=(const ChannelOutput& output) {
        this->r = output.r;
        this->g = output.g;
        this->b = output.b;
        this->ww = output.ww;
        this->cw = output.cw;
        return *this;
    }

    bool isOn() {
        return this->r > 0 || this->g > 0 || this->b > 0 || this->ww > 0 || this->cw > 0;
    }
};

// struct for HSV + Kelvin
struct HSVCT {

    union {
        int h;
        int hue;
    };
    union {
        int s;
        int sat;
        int saturation;
    };
    union {
        int v;
        int val;
        int value;
    };
    union {
        int ct;
        int colortemp;
    };

    HSVCT() :
            h(0), s(0), v(0), ct(2700) {
    }
    HSVCT(int hue, int sat, int val) :
            h(hue), s(sat), v(val), ct(0) {
    }
    HSVCT(int hue, int sat, int val, int ct) :
            h(hue), s(sat), v(val), ct(ct) {
    }

    //construct from float values
    HSVCT(float hue, float sat, float val) {
        this->h = (constrain(hue, 0.0, 360.0) / 360) * RGBWW_CALC_HUEWHEELMAX;
        this->s = (constrain(sat, 0.0, 100.0) / 100) * RGBWW_CALC_MAXVAL;
        this->v = (constrain(val, 0.0, 100.0) / 100) * RGBWW_CALC_MAXVAL;
        //TODO: default value for White color?
        this->ct = 0;
    }

    HSVCT(float hue, float sat, float val, int ct) :
            HSVCT(hue, sat, val) {
        ct = constrain(ct, 0, 10000);
    }

    HSVCT(const HSVCT& hsvk) {
        this->h = hsvk.h;
        this->s = hsvk.s;
        this->v = hsvk.v;
        this->ct = hsvk.ct;
    }

    bool operator==(const HSVCT& obj) const {
        return this->h == obj.h && this->s == obj.s && this->v == obj.v && this->ct == obj.ct;
    }

    bool operator!=(const HSVCT& obj) const {
        return !(*this == obj);
    }

    HSVCT& operator=(const HSVCT& hsvct) {
        this->h = hsvct.h;
        this->s = hsvct.s;
        this->v = hsvct.v;
        this->ct = hsvct.ct;
        return *this;
    }

    HSVCT& operator=(String& colorStr) {
        Vector<String> tokens;
        splitString(colorStr, ',', tokens);

        if (tokens.size() < 3 || tokens.size() > 4) {
            debug_e("HSVCT::setFromString - Invalid input string: %s", colorStr.c_str());
            return *this;
        }

        this->h = (constrain(tokens[0].toFloat(), 0.0, 360.0) / 360) * RGBWW_CALC_HUEWHEELMAX;
        this->s = (constrain(tokens[1].toFloat(), 0.0, 100.0) / 100) * RGBWW_CALC_MAXVAL;
        this->v = (constrain(tokens[2].toFloat(), 0.0, 100.0) / 100) * RGBWW_CALC_MAXVAL;

        if (tokens.size() > 3)
            this->colortemp = tokens[3].toInt();
        return *this;
    }

    void asRadian(float& hue, float& sat, float& val) const {
        hue = (float(h) / float(RGBWW_CALC_HUEWHEELMAX)) * 360.0;
        sat = (float(s) / float(RGBWW_CALC_MAXVAL)) * 100.0;
        val = (float(v) / float(RGBWW_CALC_MAXVAL)) * 100.0;
    }

    void asRadian(float& hue, float& sat, float& val, int& ct) const {
        hue = (float(this->h) / float(RGBWW_CALC_HUEWHEELMAX)) * 360.0;
        sat = (float(this->s) / float(RGBWW_CALC_MAXVAL)) * 100.0;
        val = (float(this->v) / float(RGBWW_CALC_MAXVAL)) * 100.0;
        ct = this->ct;
    }
};

struct RequestChannelOutput {
    Optional<AbsOrRelValue> r;
    Optional<AbsOrRelValue> g;
    Optional<AbsOrRelValue> b;
    Optional<AbsOrRelValue> ww;
    Optional<AbsOrRelValue> cw;

    RequestChannelOutput() {
    }

    RequestChannelOutput(const ChannelOutput& ch) {
        this->r = ch.r;
        this->g = ch.g;
        this->b = ch.b;
        this->cw = ch.cw;
        this->ww = ch.ww;
    }

    bool operator==(const RequestChannelOutput& ch) const {
        return this->r == ch.r && this->g == ch.g && this->b == ch.b && this->ww == ch.ww && this->cw == ch.cw;
    }

    bool operator!=(const RequestChannelOutput& obj) const {
        return !(*this == obj);
    }

    RequestChannelOutput& operator=(const RequestChannelOutput& ch) {
        this->r = ch.r;
        this->g = ch.g;
        this->b = ch.b;
        this->cw = ch.cw;
        this->ww = ch.ww;
        return *this;
    }
};

struct RequestHSVCT {
    Optional<AbsOrRelValue> h;
    Optional<AbsOrRelValue> s;
    Optional<AbsOrRelValue> v;
    Optional<AbsOrRelValue> ct;

    RequestHSVCT() {
    }

    RequestHSVCT(const HSVCT& hsvct) {
        h = hsvct.h;
        s = hsvct.s;
        v = hsvct.v;
        ct = hsvct.ct;
    }

    bool operator==(const RequestHSVCT& obj) const {
        return this->h == obj.h && this->s == obj.s && this->v == obj.v && this->ct == obj.ct;
    }

    bool operator!=(const RequestHSVCT& obj) const {
        return !(*this == obj);
    }

    RequestHSVCT& operator=(const RequestHSVCT& hsvct) {
        this->h = hsvct.h;
        this->s = hsvct.s;
        this->v = hsvct.v;
        this->ct = hsvct.ct;
        return *this;
    }

    operator HSVCT() {
        HSVCT c;
        c.h = h.getValue();
        c.s = s.getValue();
        c.v = v.getValue();
        c.ct = ct.getValue();
        return c;
    }
};

struct COLOR {
    union {
        RGBWCT rgbw;
        HSVCT hsv;
    };

    union {
        int ct;
        int colortemp;
    };
};

/**
 * Class with functions for converting between different colorspaces
 * (HSVK, RGBWK), changing outputmodes (RGBWW_COLORMODE) and
 * calculating color temperature and white balance
 *
 */
class RGBWWColorUtils {

public:
    RGBWWColorUtils();
    virtual ~RGBWWColorUtils() {
    }
    ;

    /**
     * Set the output setting of the controler.
     * See RGBWW_COLORMODE for modes
     *
     * @param mode RGBWW_COLORMODE
     */
    void setColorMode(RGBWW_COLORMODE mode);

    /**
     * Get the current output setting
     *
     * @return RGBWW_COLORMODE
     */
    RGBWW_COLORMODE getColorMode() const;

    /**
     * set the default conversion for HSVK to RGBWK color space.
     * For valid models see RGBWW_HSVMODEL
     *
     * @param model	RGBWW_HSVMODEL models
     */
    void setHSVmodel(RGBWW_HSVMODEL model);

    /**
     * Returns the model used for default conversion between hsvk and rgbwk
     *
     * @return RGBWW_HSVMODEL
     */
    RGBWW_HSVMODEL getHSVmodel() const;

    /**
     * Set the color temperature for warm/cold white channel in kelvin
     *
     * @param WarmWhite color temperatur of warm white channel in kelvin
     * @param ColdWhite color temperature of cold white channel in kelvin
     */
    void setWhiteTemperature(int WarmWhite, int ColdWhite);

    /**
     * Returns the settings for the warm/cold white color temperatures
     *
     * @param WarmWhite color temperatur of warm white channel in kelvin
     * @param ColdWhite color temperature of cold white channel in kelvin
     */
    void getWhiteTemperature(int& WarmWhite, int& ColdWhite) const;

    /**
     * Correction for HSVtoRGB Normal Mode. Moves the boundaries
     * of each color further left/right. Assumes all variables are
     * contained in [-30.0, 30.0]
     *
     * More information on HSV colorspace see:
     * https://en.wikipedia.org/wiki/HSL_and_HSV#Hue_and_chroma
     *
     * @param float	red
     * @param float	yellow
     * @param float	green
     * @param float	cyan
     * @param float	blue
     * @param float	magenta
     */
    void setHSVcorrection(float red, float yellow, float green, float cyan, float blue, float magenta);

    /**
     * Copies the current values used for HSV correction into the
     * provided params
     *
     * @param float&	red
     * @param float&	yellow
     * @param float&	green
     * @param float&	cyan
     * @param float&	blue
     * @param float&	magenta
     */
    void getHSVcorrection(float& red, float& yellow, float& green, float& cyan, float& blue, float& magenta) const;

    /**
     * Set the maximum brightness if output for the channels
     * Assumes that r,g,b,cw and ww are contained in the
     * set [0 , 100]
     *
     * @param int	r	red channel
     * @param int 	g	green channel
     * @param int	b	blue channel
     * @param int 	cw 	cold white channel
     * @param int 	ww 	warm white channel (
     *
     */
    void setBrightnessCorrection(int r, int g, int b, int ww, int cw);

    /**
     * Copies the current value of the brightness factor into the specified variables
     *
     * @param int&	r
     * @param int&	g
     * @param int&	b
     * @param int&	ww
     * @param int&	cw
     */
    void getBrightnessCorrection(int& r, int& g, int& b, int& ww, int& cw) const;

    /**
     * Applies the white colortemperature
     *
     * @param RGBWK&	rgbw
     * @param int& 		ww
     * @param int&		cw
     */
    void whiteBalance(RGBWCT& rgbw, ChannelOutput& output) const;

    /**
     * Corrects the values in the parsed array according to the set
     * brightness correction
     *
     * @param int[] color
     */
    void correctBrightness(ChannelOutput& output) const;

    /**
     * Convert HSVK Values to RGBK colorspace
     * Uses to conversion model set with setHSVmodel
     *
     * @param hsvk		HSVK struct with values
     * @param rgbwk		RGBWK struct to hold result
     */
    void HSVtoRGB(const HSVCT& hsvk, RGBWCT& rgbwk) const;

    /**
     * Convert HSVK Values to RGBK colorspace
     *
     * @param hsvk		HSVK struct with values
     * @param rgbwk		RGBWK struct to hold result
     * @param mode		conversion model to be used (RGBWW_HSVMODEL)
     */
    void HSVtoRGB(const HSVCT& hsvk, RGBWCT& rgbwk, RGBWW_HSVMODEL mode) const;

    /**
     * Convert HSV values to RGB colorspace using the algorithm
     * from https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV
     *
     *
     * @param hsvk		HSVK struct with values
     * @param rgbwk		RGBWK struct to hold result
     */
    void HSVtoRGBraw(const HSVCT& hsvk, RGBWCT& rgbwk) const;

    /**
     * Convert HSV values to RGB colorspace by keeping
     * the max total color output equal.
     * Information see:
     * https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors
     *
     * @param hsvk		HSVK struct with values
     * @param rgbwk		RGBWK struct to hold result
     */
    void HSVtoRGBspektrum(const HSVCT& hsvk, RGBWCT& rgbwk) const;

    /**
     * Convert HSV values to RGB colorspace with rainbow color table
     * Information see:
     * https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors
     *
     * @param hsvk		HSVK struct with values
     * @param rgbwk		RGBWK struct to hold result
     */
    void HSVtoRGBrainbow(const HSVCT& hsvk, RGBWCT& rgbwk) const;

    /**
     *
     * @param rgbwk
     * @param hsvk
     */
    void RGBtoHSV(const RGBWCT& rgbwk, HSVCT& hsvk) const;

    /**
     * Helper function to keep HUE within boundaries [0, HUELWHEELMAX]
     *
     * @param hue
     */
    static void circleHue(int& hue);

private:
    int _BrightnessFactor[RGBWW_CHANNELS::NUM_CHANNELS];
    int _HueWheelSector[7];
    int _HueWheelSectorWidth[6];
    int _WarmWhiteKelvin;
    int _ColdWhiteKelvin;

    RGBWW_COLORMODE _colormode;
    RGBWW_HSVMODEL _hsvmodel;

    static int parseColorCorrection(float val);
    void createHueWheel();

};
