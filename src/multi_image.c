#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define TOTAL_SLIDES 26  //TOTAL is the total number of images in the data setup below. Important to get this right.

#define MY_UUID { 0x8C, 0x0C, 0x3B, 0x6E, 0xF9, 0xFA, 0x41, 0x8F, 0xBC, 0xFE, 0x25, 0x81, 0xCB, 0x5E, 0xFC, 0xAD }
PBL_APP_INFO(MY_UUID, "MultiImage", "Glenn Loos-Austin", 1, 1 /* App version */, RESOURCE_ID_MENU_ICON, APP_INFO_STANDARD_APP);

//Some of the initial code for this project comes from the Family source code by Darren Oakey,
//which provided an excellent starting point for this project.

Window window;
TextLayer _currentDate;
TextLayer _currentTime;
BitmapLayer _currentPicture;
int _totalImages = TOTAL_SLIDES;  //used to do some boundary checking, and to run an initialization loop.

BmpContainer _image0; // holds our images

int _bmpResourceID[TOTAL_SLIDES];  //These 4 arrays should probably be converted to object properties.
int _targetImage[TOTAL_SLIDES];    
int _nextImage[TOTAL_SLIDES];
int _prevImage[TOTAL_SLIDES];

//tracking globals

int _currentImage=0;    //the index of the image that is currently displayed.
bool _hidingTime=true; //indicates whether or not the time should be currently hidden. true=not showing. false=showing.

//
// sets up a text layer
// (what layer, what window, x coord, y coord, width, height, text color, background color, text alignment)
//
void setupTextLayer( TextLayer *layer, Window *parent, int x, int y, int width, int height, GFont font, GColor colorChoice, GColor bgChoice, GTextAlignment whatAlign )
{
    text_layer_init(layer, window.layer.frame);
    text_layer_set_text_color(layer, colorChoice);
    text_layer_set_background_color(layer, bgChoice);
    text_layer_set_text_alignment(layer, whatAlign);
    layer_set_frame(&layer->layer, GRect(x, y, width, height));
    text_layer_set_font(layer, font );
    layer_add_child(&parent->layer, &layer->layer);
}

//
//only one image should be inited, so we figure out whether it's _image1 or _image2 and deinit it.
//
void handle_deinit(AppContextRef ctx) {
    (void)ctx;
    bmp_deinit_container(&_image0);    
}


//
//Set container image function
//attempt to only have one image container inited at any given time, so as to avoid memory leaks/out of memory.
//previously tried to switch back and forth, now using a technique I observed in the 91dub example app, from
// where the following function comes.
//
void set_container_image(BmpContainer *bmp_container, const int resource_id, GPoint origin) {
    
    layer_remove_from_parent(&bmp_container->layer.layer);
    bmp_deinit_container(bmp_container);
    
    bmp_init_container(resource_id, bmp_container);
    
    GRect frame = layer_get_frame(&bmp_container->layer.layer);
    frame.origin.x = origin.x;
    frame.origin.y = origin.y;
    layer_set_frame(&bmp_container->layer.layer, frame);
    
    layer_add_child(&_currentPicture.layer, &bmp_container->layer.layer);
}


//
//Main screen drawing routine
//now very dull, calls set_container_image.
//
void ScreenChange(int target) {    
    set_container_image(&_image0,_bmpResourceID[target],GPoint(0, 0));
}

//
// Click config handlers
//
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    if (_prevImage[_currentImage] == -1)
    {
        _currentImage--;
        if (_currentImage<0) _currentImage = _totalImages; //probably not needed, but boundary checking just in case.
    }
    else
    {
        _currentImage = _prevImage[_currentImage];
    }
    ScreenChange(_currentImage);
}

void up_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
    _currentImage = 0;
    ScreenChange(_currentImage);
}


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
    if (_nextImage[_currentImage] == -1)
    {
        _currentImage++;
        if (_currentImage >= _totalImages) _currentImage = 0;  //probably not needed, but boundary checking just in case.
    }
    else
    {
        _currentImage = _nextImage[_currentImage];
    }
    ScreenChange(_currentImage);
}

void select_click_handler(ClickRecognizerRef recognizer, Window *window) {
    if (_targetImage[_currentImage] == -1)
    {
        //vibrate
        vibes_short_pulse();
    }
    else
    {
        _currentImage = _targetImage[_currentImage];
    }
    ScreenChange(_currentImage);
}

void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
    _hidingTime = !_hidingTime;
    layer_set_hidden(&_currentTime.layer, _hidingTime);
    layer_set_hidden(&_currentDate.layer, _hidingTime);
}

//
// Click config provider
//
void config_provider(ClickConfig **config, Window *window) {
    // See ui/click.h for more information and default values.
    
    config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
    config[BUTTON_ID_UP]->long_click.handler = (ClickHandler) up_long_click_handler;
    config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
    config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_click_handler;
    config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;
    
    (void)window;
}


//
// sets up the display
//
void handle_init(AppContextRef ctx) {
  (void)ctx;

    window_init(&window, "Simplicity");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);
    window_set_fullscreen(&window, true);
    resource_init_current_app(&APP_RESOURCES);
    
    window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);
    
    //
    // Initialize the bitmap linking arrays to link nowhere.
    //
    for(int i=0;i<_totalImages;i++)
    {
        _targetImage[i]=-1;
        _nextImage[i]=-1;
        _prevImage[i]=-1;
    }
    
    //
    //Slide group setup
    //
    //id is incremented for each resourceID we set.
    //remember to set the 4 array sizes to to number of images up in the global declarations.
    //
    
    int id=0; //start with ID 0.
    
    _bmpResourceID[id] = RESOURCE_ID_NAV_IMAGE_001;
        _targetImage[id] = id+7; //Fandom                   //fandom is 7 slides forward. select should take us there.
        _prevImage[id] = id+4;                              //for slide 0, up button should jump four slides forward to wrap menu.
        bmp_init_container( _bmpResourceID[id], &_image0);  //assign to _image0 so that inital screendraw can happen.
    _bmpResourceID[++id] = RESOURCE_ID_NAV_IMAGE_002;
        _targetImage[id] = id+15; //fantasy                 //fantasy is currently 13 slides forward. select should take us there.
    _bmpResourceID[++id] = RESOURCE_ID_NAV_IMAGE_003;
        _targetImage[id] = id+11; //family                   //alas, these offsets are somewhat fragile.
    _bmpResourceID[++id] = RESOURCE_ID_NAV_IMAGE_004;
        _targetImage[id] = id+18; //mockups
    _bmpResourceID[++id] = RESOURCE_ID_NAV_IMAGE_005;
        _targetImage[id] = id+1;  //help
        _nextImage[id] = id-4;                              //for slide 4, down button should jump four slides backward to wrap menu.

    //Help Group
    _bmpResourceID[++id] = RESOURCE_ID_HELP_IMAGE_001;
        _prevImage[id] = id+1;
    _bmpResourceID[++id] = RESOURCE_ID_HELP_IMAGE_002;
        _nextImage[id] = id-1;

    //Fandom Group
    _bmpResourceID[++id] = RESOURCE_ID_FANDOM_IMAGE_001;
        _prevImage[id] = id+5;
    _bmpResourceID[++id] = RESOURCE_ID_FANDOM_IMAGE_002;
    _bmpResourceID[++id] = RESOURCE_ID_FANDOM_IMAGE_003;
    _bmpResourceID[++id] = RESOURCE_ID_FANDOM_IMAGE_004;
    _bmpResourceID[++id] = RESOURCE_ID_FANDOM_IMAGE_005;
    _bmpResourceID[++id] = RESOURCE_ID_FANDOM_IMAGE_006;
        _nextImage[id] = id-5;
    
    //Family Group
    _bmpResourceID[++id] = RESOURCE_ID_FAMILY_IMAGE_001;
        _prevImage[id] = id+2;
    _bmpResourceID[++id] = RESOURCE_ID_FAMILY_IMAGE_002;
    _bmpResourceID[++id] = RESOURCE_ID_FAMILY_IMAGE_003;
        _nextImage[id] = id-2;
 
    //Fantasy Group
    _bmpResourceID[++id] = RESOURCE_ID_FANTASY_IMAGE_001;
        _prevImage[id] = id+4;
    _bmpResourceID[++id] = RESOURCE_ID_FANTASY_IMAGE_002;
    _bmpResourceID[++id] = RESOURCE_ID_FANTASY_IMAGE_003;
    _bmpResourceID[++id] = RESOURCE_ID_FANTASY_IMAGE_004;
    _bmpResourceID[++id] = RESOURCE_ID_FANTASY_IMAGE_005;
        _nextImage[id] = id-4;

    //Mockups Group
    _bmpResourceID[++id] = RESOURCE_ID_MOCKUP_IMAGE_001;
    _prevImage[id] = id+4;
    _bmpResourceID[++id] = RESOURCE_ID_MOCKUP_IMAGE_002;
    _bmpResourceID[++id] = RESOURCE_ID_MOCKUP_IMAGE_003;
    _bmpResourceID[++id] = RESOURCE_ID_MOCKUP_IMAGE_004;
    _bmpResourceID[++id] = RESOURCE_ID_MOCKUP_IMAGE_005;
    _nextImage[id] = id-4;

    //
    //Slide group setup end
    //
    
    
    //
    // create the bitmap layer at the back
    //
    bitmap_layer_init( &_currentPicture, GRect(0,0, 144, 168) );
    layer_add_child( &window.layer, &_currentPicture.layer );
    
    ScreenChange(0); // do initial screen draw.
    
    //
    // set the font we are using for time display.
    //
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    GFont font2 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    
    //fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21));
    
    //
    // create the text layers
    //
    setupTextLayer( &_currentDate, &window, 0, 168-23, 144, 23, font2, GColorWhite, GColorBlack, GTextAlignmentRight);
    setupTextLayer( &_currentTime, &window, 0, 168-28, 144, 28, font, GColorWhite, GColorClear, GTextAlignmentLeft);
    
    layer_set_hidden(&_currentTime.layer, _hidingTime);
    layer_set_hidden(&_currentDate.layer, _hidingTime);
}


//
// ticks every minute, updating the time and date.
//
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t)
{
    //
    // this seems pointless.
    // Glenn: Left this in because I don't know what it does.
    //
    (void)ctx;

    PblTm *tickTime = t->tick_time;
   
    //
    // set the date - only changing it when the day changes
    // format strings here: http://www.gnu.org/software/emacs/manual/html_node/elisp/Time-Parsing.html
    //
    static char dateText[] = "Sun 01 September 00";
    static int lastShownDate = -1;
    int theDay = tickTime->tm_yday;
    if (theDay != lastShownDate)
    {
        lastShownDate = theDay;
        string_format_time(dateText, sizeof(dateText), "%a %B %e", tickTime );
        text_layer_set_text(&_currentDate, dateText);
    }
    
    //
    // set the time
    //
    static char timeText[] = "00:00";
    const char *timeFormat = clock_is_24h_style() ? "%R" : "%l:%M";
    string_format_time(timeText, sizeof(timeText), timeFormat, tickTime);
    text_layer_set_text(&_currentTime, timeText);
}

//
// main
//
void pbl_main(void *params)
{
    PebbleAppHandlers handlers =
    {
        .init_handler = &handle_init,
        .deinit_handler = &handle_deinit,

        .tick_info =
        {
            .tick_handler = &handle_minute_tick,
            .tick_units = MINUTE_UNIT
        }
    };
    app_event_loop(params, &handlers);
}
