#include "luaapplication.h"

#include "eventdispatcher.h"

#include "platform.h"

#include "application.h"

#include "luautil.h"
#include "stackchecker.h"

#include "binder.h"
#include "eventbinder.h"
#include "eventdispatcherbinder.h"
#include "enterframeevent.h"
#include "spritebinder.h"
#include "matrixbinder.h"
#include "texturebasebinder.h"
#include "texturebinder.h"
#include "texturepackbinder.h"
#include "bitmapdatabinder.h"
#include "bitmapbinder.h"
#include "stagebinder.h"
#include "timerbinder.h"
#include "fontbasebinder.h"
#include "fontbinder.h"
#include "ttfontbinder.h"
#include "textfieldbinder.h"
#include "accelerometerbinder.h"
#include "box2dbinder2.h"
#include "dibbinder.h"
#include "applicationbinder.h"
#include "tilemapbinder.h"
#include "shapebinder.h"
#include "movieclipbinder.h"
#include "urlloaderbinder.h"
#include "geolocationbinder.h"
#include "gyroscopebinder.h"
#include "timer.h"
#include "alertdialogbinder.h"
#include "textinputdialogbinder.h"
#include "meshbinder.h"
#include "audiobinder.h"
#include "rendertargetbinder.h"
#include "stageorientationevent.h"

#include "keys.h"

#include "ogl.h"

#include <algorithm>

#include <gfile.h>
#include <gfile_p.h>

#include <pluginmanager.h>

#include <keycode.h>

#include <glog.h>

#include <gevent.h>
#include <ginput.h>
#include <gapplication.h>

#include "tlsf.h"

const char* LuaApplication::fileNameFunc_s(const char* filename, void* data)
{
	LuaApplication* that = static_cast<LuaApplication*>(data);
	return that->fileNameFunc(filename);
}

const char* LuaApplication::fileNameFunc(const char* filename)
{
	return g_pathForFile(filename);
}

/*
static const char* sound_lua = 
"function Sound:play(startTime, loops)"						"\n"
"	return SoundChannel.new(self, startTime, loops)"		"\n"
"end"														"\n"
;

static const char* texturepack_lua = 
"function TexturePack:getBitmapData(index)"					"\n"
"	local x, y, width, height"								"\n"
"	x, y, width, height = self:getLocation(index)"			"\n"
""															"\n"
"	if x == nil then"										"\n"
"		return nil"											"\n"
"	end"													"\n"
""															"\n"
"	return BitmapData.new(self, x, y, width, height)"		"\n"
"end"														"\n"
;

static const char* class_lua = 
"function class(b)"											"\n"
"	local c = {}"											"\n"
"	setmetatable(c, b)"										"\n"
"	c.__index = c"											"\n"
""															"\n"
"	c.new = function(...)"									"\n"
"		local s0 = b.new(...)"								"\n"
"		local s1 = {}"										"\n"
"		setmetatable(s1, c)"								"\n"
""															"\n"
"		for k,v in pairs(s0) do"							"\n"
"			s1[k] = v"										"\n"
"		end"												"\n"
""															"\n"
"		return s1"											"\n"
"	end"													"\n"
""															"\n"
"	return c"												"\n"
"end"														"\n"
;

static const char* class_v2_lua = 
"function class_v2(b)"										"\n"
"	local c = {}"											"\n"
"	setmetatable(c, b)"										"\n"
"	c.__index = c"											"\n"
""															"\n"
"	c.new = function(...)"									"\n"
"		local s0 = b.new(...)"								"\n"
"		setmetatable(s0, c)"								"\n"
"		s0.__index = s0"									"\n"
""															"\n"
"		local s1 = {}"										"\n"
"		setmetatable(s1, s0)"								"\n"
""															"\n"
"		return s1"											"\n"
"	end"													"\n"
""															"\n"
"	return c"												"\n"
"end"														"\n"
;
*/
static int environTable(lua_State* L)
{
	StackChecker checker(L, "environTable", 1);

	static char k = ' ';		// todo: maybe we address the table with this pointer

	lua_pushlightuserdata(L, &k);
	lua_rawget(L, LUA_REGISTRYINDEX);
	
	if (lua_isnil(L, -1) == 1)
	{
		lua_pop(L, 1);			// pop nil

		lua_pushlightuserdata(L, &k);
		lua_newtable(L);
		lua_rawset(L, LUA_REGISTRYINDEX);

		lua_pushlightuserdata(L, &k);
		lua_rawget(L, LUA_REGISTRYINDEX);
		
		//luaL_newweaktable(L);
		//lua_setfield(L, -2, "bridges");

		lua_newtable(L);
		lua_setfield(L, -2, "timers");

		lua_newtable(L);
		lua_setfield(L, -2, "soundchannels");
	}

	return 1;
}

int setEnvironTable(lua_State* L)
{
	return 0;

	StackChecker checker(L, "setEnvironTable", 0);

	environTable(L);
	lua_replace(L, LUA_ENVIRONINDEX);

	return 0;
}

static int os_timer(lua_State* L)
{
	lua_pushnumber(L, iclock());
	return 1;
}


void registerModules(lua_State* L);

static int bindAll(lua_State* L)
{
	Application* application = static_cast<Application*>(lua_touserdata(L, 1));
	lua_pop(L, 1);

	StackChecker checker(L, "bindAll", 0);

	setEnvironTable(L);

	{
		lua_newtable(L);
		luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_touches);

		lua_newtable(L);
		luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_eventClosures);

		lua_newtable(L);
		luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_events);

		luaL_newweaktable(L);
		luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_b2);

		lua_newtable(L);
		luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_timers);
	}

    luaL_newmetatable(L, "Object");
    lua_setglobal(L, "Object");
	
	EventBinder eventBinder(L);
	EventDispatcherBinder eventDispatcherBinder(L);
	TimerBinder timerBinder(L);
	MatrixBinder matrixBinder(L);
	SpriteBinder spriteBinder(L);
	TextureBaseBinder textureBaseBinder(L);
	TextureBinder textureBinder(L);
	TexturePackBinder texturePackBinder(L);
	BitmapDataBinder bitmapDataBinder(L);
	BitmapBinder bitmapBinder(L);
	StageBinder stageBinder(L, application);
	FontBaseBinder fontBaseBinder(L);
	FontBinder fontBinder(L);
	TTFontBinder ttfontBinder(L);
	TextFieldBinder textFieldBinder(L);
    AccelerometerBinder accelerometerBinder(L);
	Box2DBinder2 box2DBinder2(L);
	DibBinder dibBinder(L);
	TileMapBinder tileMapBinder(L);
	ApplicationBinder applicationBinder(L);
	ShapeBinder shapeBinder(L);
	MovieClipBinder movieClipBinder(L);
    UrlLoaderBinder urlLoaderBinder(L);
	GeolocationBinder geolocationBinder(L);
	GyroscopeBinder gyroscopeBinder(L);
    AlertDialogBinder alertDialogBinder(L);
    TextInputDialogBinder textInputDialogBinder(L);
    MeshBinder meshBinder(L);
    AudioBinder audioBinder(L);
    RenderTargetBinder renderTargetBinder(L);

	PluginManager& pluginManager = PluginManager::instance();
	for (size_t i = 0; i < pluginManager.plugins.size(); ++i)
        pluginManager.plugins[i].main(L, 0);

	lua_getglobal(L, "Event");
	lua_getfield(L, -1, "new");
	lua_pushlightuserdata(L, NULL);
	lua_call(L, 1, 1);
	lua_remove(L, -2);
	luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_Event);

	lua_getglobal(L, "Event");
	lua_getfield(L, -1, "new");
	lua_pushlightuserdata(L, NULL);
	lua_call(L, 1, 1);
	lua_remove(L, -2);
	luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_EnterFrameEvent);

	lua_getglobal(L, "Event");
	lua_getfield(L, -1, "new");
	lua_pushlightuserdata(L, NULL);
	lua_call(L, 1, 1);
	lua_remove(L, -2);
	luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_MouseEvent);

	lua_getglobal(L, "Event");
	lua_getfield(L, -1, "new");
	lua_pushlightuserdata(L, NULL);
	lua_call(L, 1, 1);
	lua_remove(L, -2);
	luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_TouchEvent);


	lua_getglobal(L, "Event");
	lua_getfield(L, -1, "new");
	lua_pushlightuserdata(L, NULL);
	lua_call(L, 1, 1);
	lua_remove(L, -2);
	luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_TimerEvent);

	lua_getglobal(L, "Event");
	lua_getfield(L, -1, "new");
	lua_pushlightuserdata(L, NULL);
	lua_call(L, 1, 1);
	lua_remove(L, -2);
	luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_KeyboardEvent);

    lua_getglobal(L, "Event");
    lua_getfield(L, -1, "new");
    lua_pushlightuserdata(L, NULL);
    lua_call(L, 1, 1);
    lua_remove(L, -2);
    luaL_rawsetptr(L, LUA_REGISTRYINDEX, &key_CompleteEvent);

#include "property.c.in"
#include "texturepack.c.in"
#include "sprite.c.in"
#include "compatibility.c.in"

	lua_newtable(L);

    lua_pushinteger(L, GINPUT_KEY_BACK);
	lua_setfield(L, -2, "BACK");
    lua_pushinteger(L, GINPUT_KEY_SEARCH);
	lua_setfield(L, -2, "SEARCH");
    lua_pushinteger(L, GINPUT_KEY_MENU);
	lua_setfield(L, -2, "MENU");
    lua_pushinteger(L, GINPUT_KEY_CENTER);
	lua_setfield(L, -2, "CENTER");
    lua_pushinteger(L, GINPUT_KEY_SELECT);
	lua_setfield(L, -2, "SELECT");
    lua_pushinteger(L, GINPUT_KEY_START);
	lua_setfield(L, -2, "START");
    lua_pushinteger(L, GINPUT_KEY_L1);
	lua_setfield(L, -2, "L1");
    lua_pushinteger(L, GINPUT_KEY_R1);
	lua_setfield(L, -2, "R1");

    lua_pushinteger(L, GINPUT_KEY_LEFT);
	lua_setfield(L, -2, "LEFT");
    lua_pushinteger(L, GINPUT_KEY_UP);
	lua_setfield(L, -2, "UP");
    lua_pushinteger(L, GINPUT_KEY_RIGHT);
	lua_setfield(L, -2, "RIGHT");
    lua_pushinteger(L, GINPUT_KEY_DOWN);
	lua_setfield(L, -2, "DOWN");

    lua_pushinteger(L, GINPUT_KEY_A);
    lua_setfield(L, -2, "A");
    lua_pushinteger(L, GINPUT_KEY_B);
    lua_setfield(L, -2, "B");
    lua_pushinteger(L, GINPUT_KEY_C);
    lua_setfield(L, -2, "C");
    lua_pushinteger(L, GINPUT_KEY_D);
    lua_setfield(L, -2, "D");
    lua_pushinteger(L, GINPUT_KEY_E);
    lua_setfield(L, -2, "E");
    lua_pushinteger(L, GINPUT_KEY_F);
    lua_setfield(L, -2, "F");
    lua_pushinteger(L, GINPUT_KEY_G);
    lua_setfield(L, -2, "G");
    lua_pushinteger(L, GINPUT_KEY_H);
    lua_setfield(L, -2, "H");
    lua_pushinteger(L, GINPUT_KEY_I);
    lua_setfield(L, -2, "I");
    lua_pushinteger(L, GINPUT_KEY_J);
    lua_setfield(L, -2, "J");
    lua_pushinteger(L, GINPUT_KEY_K);
    lua_setfield(L, -2, "K");
    lua_pushinteger(L, GINPUT_KEY_L);
    lua_setfield(L, -2, "L");
    lua_pushinteger(L, GINPUT_KEY_M);
    lua_setfield(L, -2, "M");
    lua_pushinteger(L, GINPUT_KEY_N);
    lua_setfield(L, -2, "N");
    lua_pushinteger(L, GINPUT_KEY_O);
    lua_setfield(L, -2, "O");
    lua_pushinteger(L, GINPUT_KEY_P);
    lua_setfield(L, -2, "P");
    lua_pushinteger(L, GINPUT_KEY_Q);
    lua_setfield(L, -2, "Q");
    lua_pushinteger(L, GINPUT_KEY_R);
    lua_setfield(L, -2, "R");
    lua_pushinteger(L, GINPUT_KEY_S);
    lua_setfield(L, -2, "S");
    lua_pushinteger(L, GINPUT_KEY_T);
    lua_setfield(L, -2, "T");
    lua_pushinteger(L, GINPUT_KEY_U);
    lua_setfield(L, -2, "U");
    lua_pushinteger(L, GINPUT_KEY_V);
    lua_setfield(L, -2, "V");
    lua_pushinteger(L, GINPUT_KEY_W);
    lua_setfield(L, -2, "W");
    lua_pushinteger(L, GINPUT_KEY_X);
    lua_setfield(L, -2, "X");
    lua_pushinteger(L, GINPUT_KEY_Y);
    lua_setfield(L, -2, "Y");
    lua_pushinteger(L, GINPUT_KEY_Z);
    lua_setfield(L, -2, "Z");

    lua_pushinteger(L, GINPUT_KEY_0);
    lua_setfield(L, -2, "NUM_0");
    lua_pushinteger(L, GINPUT_KEY_1);
    lua_setfield(L, -2, "NUM_1");
    lua_pushinteger(L, GINPUT_KEY_2);
    lua_setfield(L, -2, "NUM_2");
    lua_pushinteger(L, GINPUT_KEY_3);
    lua_setfield(L, -2, "NUM_3");
    lua_pushinteger(L, GINPUT_KEY_4);
    lua_setfield(L, -2, "NUM_4");
    lua_pushinteger(L, GINPUT_KEY_5);
    lua_setfield(L, -2, "NUM_5");
    lua_pushinteger(L, GINPUT_KEY_6);
    lua_setfield(L, -2, "NUM_6");
    lua_pushinteger(L, GINPUT_KEY_7);
    lua_setfield(L, -2, "NUM_7");
    lua_pushinteger(L, GINPUT_KEY_8);
    lua_setfield(L, -2, "NUM_8");
    lua_pushinteger(L, GINPUT_KEY_9);
    lua_setfield(L, -2, "NUM_9");

	lua_setglobal(L, "KeyCode");


	// correct clock function which is wrong in iphone
	lua_getglobal(L, "os");
	lua_pushcfunction(L, os_timer);
	lua_setfield(L, -2, "timer");
	lua_pop(L, 1);

	// register collectgarbagelater
//	lua_pushcfunction(L, ::collectgarbagelater);
//	lua_setglobal(L, "collectgarbagelater");

	registerModules(L);

	return 0;
}

LuaApplication::LuaApplication(void)
{
    L = luaL_newstate();
	printFunc_ = lua_getprintfunc(L);
    printData_ = NULL;
	lua_close(L);
	L = NULL;

	application_ = 0;

    isPlayer_ = true;

	exceptionsEnabled_ = true;

	orientation_ = ePortrait;

	width_ = 320;
	height_ = 480;
	scale_ = 1;

    ginput_addCallback(callback_s, this);
    gapplication_addCallback(callback_s, this);
}

void LuaApplication::callback_s(int type, void *event, void *udata)
{
    static_cast<LuaApplication*>(udata)->callback(type, event);
}

void LuaApplication::callback(int type, void *event)
{
    if (type == GINPUT_MOUSE_DOWN_EVENT)
    {
        ginput_MouseEvent *event2 = (ginput_MouseEvent*)event;
        application_->mouseDown(event2->x, event2->y);
    }
    else if (type == GINPUT_MOUSE_MOVE_EVENT)
    {
        ginput_MouseEvent *event2 = (ginput_MouseEvent*)event;
        application_->mouseMove(event2->x, event2->y);
    }
    else if (type == GINPUT_MOUSE_UP_EVENT)
    {
        ginput_MouseEvent *event2 = (ginput_MouseEvent*)event;
        application_->mouseUp(event2->x, event2->y);
    }
    else if (type == GINPUT_KEY_DOWN_EVENT)
    {
        ginput_KeyEvent *event2 = (ginput_KeyEvent*)event;
        application_->keyDown(event2->keyCode, event2->realCode);
    }
    else if (type == GINPUT_KEY_UP_EVENT)
    {
        ginput_KeyEvent *event2 = (ginput_KeyEvent*)event;
        application_->keyUp(event2->keyCode, event2->realCode);
    }
    else if (type == GINPUT_TOUCH_BEGIN_EVENT)
    {
        ginput_TouchEvent *event2 = (ginput_TouchEvent*)event;
        application_->touchesBegin(event2);
    }
    else if (type == GINPUT_TOUCH_MOVE_EVENT)
    {
        ginput_TouchEvent *event2 = (ginput_TouchEvent*)event;
        application_->touchesMove(event2);
    }
    else if (type == GINPUT_TOUCH_END_EVENT)
    {
        ginput_TouchEvent *event2 = (ginput_TouchEvent*)event;
        application_->touchesEnd(event2);
    }
    else if (type == GINPUT_TOUCH_CANCEL_EVENT)
    {
        ginput_TouchEvent *event2 = (ginput_TouchEvent*)event;
        application_->touchesCancel(event2);
    }
    else if (type == GAPPLICATION_PAUSE_EVENT)
    {
        PluginManager& pluginManager = PluginManager::instance();
        for (size_t i = 0; i < pluginManager.plugins.size(); ++i)
            if (pluginManager.plugins[i].foreground)
                pluginManager.plugins[i].suspend(L);

        Event event(Event::APPLICATION_SUSPEND);
        application_->broadcastEvent(&event);
    }
    else if (type == GAPPLICATION_RESUME_EVENT)
    {
        PluginManager& pluginManager = PluginManager::instance();
        for (size_t i = 0; i < pluginManager.plugins.size(); ++i)
            if (pluginManager.plugins[i].foreground)
                pluginManager.plugins[i].resume(L);

        Event event(Event::APPLICATION_RESUME);
        application_->broadcastEvent(&event);
    }
    else if (type == GAPPLICATION_BACKGROUND_EVENT)
    {
        PluginManager& pluginManager = PluginManager::instance();
        for (size_t i = 0; i < pluginManager.plugins.size(); ++i)
            if (pluginManager.plugins[i].foreground)
                pluginManager.plugins[i].background(L);

        Event event(Event::APPLICATION_BACKGROUND);
        application_->broadcastEvent(&event);
    }
    else if (type == GAPPLICATION_FOREGROUND_EVENT)
    {
        PluginManager& pluginManager = PluginManager::instance();
        for (size_t i = 0; i < pluginManager.plugins.size(); ++i)
            if (pluginManager.plugins[i].foreground)
                pluginManager.plugins[i].foreground(L);

        Event event(Event::APPLICATION_FOREGROUND);
        application_->broadcastEvent(&event);
    }
    else if (type == GAPPLICATION_OPEN_URL_EVENT)
    {
        gapplication_OpenUrlEvent *event2 = (gapplication_OpenUrlEvent*)event;

        PluginManager& pluginManager = PluginManager::instance();
        for (size_t i = 0; i < pluginManager.plugins.size(); ++i)
            if (pluginManager.plugins[i].openUrl)
                pluginManager.plugins[i].openUrl(L, event2->url);
    }
    else if (type == GAPPLICATION_MEMORY_LOW_EVENT)
    {
        Event event(Event::MEMORY_WARNING);
        application_->broadcastEvent(&event);

        lua_gc(L, LUA_GCCOLLECT, 0);
        lua_gc(L, LUA_GCCOLLECT, 0);
    }
    else if (type == GAPPLICATION_START_EVENT)
    {
        Event event(Event::APPLICATION_START);
        application_->broadcastEvent(&event);
    }
    else if (type == GAPPLICATION_EXIT_EVENT)
    {
        Event event(Event::APPLICATION_EXIT);
        application_->broadcastEvent(&event);
    }
    else if (type == GAPPLICATION_ORIENTATION_CHANGE_EVENT)
    {
        gapplication_OrientationChangeEvent *event2 = (gapplication_OrientationChangeEvent*)event;
        StageOrientationEvent event(StageOrientationEvent::ORIENTATION_CHANGE, event2->orientation);
        application_->broadcastEvent(&event);
    }
}

/*
static std::map<std::pair<std::string, int>, double> memmap;
static double lastmem = 0;

static void testHook(lua_State* L, lua_Debug* ar)
{
	double m = lua_gc(L, LUA_GCCOUNT, 0) + lua_gc(L, LUA_GCCOUNTB, 0) / 1024.0;
	double deltam = m - lastmem;
	lastmem = m;

	lua_getinfo(L, "S", ar);
//	printf("%s:%d %g\n", ar->source, ar->currentline, deltam);
	memmap[std::make_pair(std::string(ar->source), ar->currentline)] += deltam;
}


static void maxmem()
{
	std::vector<std::pair<double, std::pair<std::string, int> > > v;

	std::map<std::pair<std::string, int>, double>::iterator iter;
	for (iter = memmap.begin(); iter != memmap.end(); ++iter)
		v.push_back(std::make_pair(iter->second, iter->first));

	std::sort(v.begin(), v.end());
	std::reverse(v.begin(), v.end());

	for (std::size_t i = 0; i < v.size(); ++i)
	{
		printf("%s:%d %g\n", v[i].second.first.c_str(), v[i].second.second, v[i].first);
		if (i == 5)
			break;
	}

	memmap.clear();
}
*/


static tlsf_t memory_pool = NULL;
static void *memory_pool_end = NULL;

static void g_free(void *ptr)
{
    if (memory_pool <= ptr && ptr < memory_pool_end)
        tlsf_free(memory_pool, ptr);
    else
        ::free(ptr);
}

static void *g_realloc(void *ptr, size_t osize, size_t size)
{
    void* p = NULL;

    if (ptr && size == 0)
    {
        g_free(ptr);
    }
    else if (ptr == NULL)
    {
        if (size <= 256)
            p = tlsf_malloc(memory_pool, size);

        if (p == NULL)
            p = ::malloc(size);
    }
    else
    {
        if (memory_pool <= ptr && ptr < memory_pool_end)
        {
            if (size <= 256)
                p = tlsf_realloc(memory_pool, ptr, size);

            if (p == NULL)
            {
                p = ::malloc(size);
                memcpy(p, ptr, osize);
                tlsf_free(memory_pool, ptr);
            }
        }
        else
        {
            p = ::realloc(ptr, size);
        }
    }

    return p;
}

#if 0
static void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void)ud;
    (void)osize;
    if (nsize == 0)
    {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, nsize);
}
#else
static void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (memory_pool == NULL)
    {
        const size_t mpsize = 1024 * 1024;
        glog_v("init_memory_pool: %dKb", mpsize / 1024);
        memory_pool = tlsf_create_with_pool(malloc(mpsize), mpsize);
        memory_pool_end = (char*)memory_pool + mpsize;
    }

    if (nsize == 0)
    {
        g_free(ptr);
        return NULL;
    }
    else
        return g_realloc(ptr, osize, nsize);
}
#endif

//int renderScene(lua_State* L);
//int mouseDown(lua_State* L);
//int mouseMove(lua_State* L);
//int mouseUp(lua_State* L);
//int touchesBegan(lua_State* L);
//int touchesMoved(lua_State* L);
//int touchesEnded(lua_State* L);
//int touchesCancelled(lua_State* L);



static int callFile(lua_State* L)
{
	StackChecker checker(L, "callFile", -1);

	setEnvironTable(L);

	luaL_rawgetptr(L, LUA_REGISTRYINDEX, &key_events);
	luaL_nullifytable(L, -1);
	lua_pop(L, 1);

	lua_call(L, 0, 0);

	return 0;
}

void LuaApplication::loadFile(const char* filename, GStatus *status)
{
    StackChecker checker(L, "loadFile", 0);

    void *pool = application_->createAutounrefPool();

    lua_pushcfunction(L, ::callFile);

    if (luaL_loadfile(L, filename))
	{
		if (exceptionsEnabled_ == true)
		{
            if (status)
                *status = GStatus(1, lua_tostring(L, -1));
		}
        lua_pop(L, 2);
        application_->deleteAutounrefPool(pool);
        return;
	}

    if (lua_pcall_traceback(L, 1, 0, 0))
	{
		if (exceptionsEnabled_ == true)
		{
            if (status)
                *status = GStatus(1, lua_tostring(L, -1));
		}
        lua_pop(L, 1);
        application_->deleteAutounrefPool(pool);
        return;
    }
    application_->deleteAutounrefPool(pool);
}


LuaApplication::~LuaApplication(void)
{
//	Referenced::emptyPool();
    ginput_removeCallback(callback_s, this);
    gapplication_removeCallback(callback_s, this);
}

void LuaApplication::deinitialize()
{
	/*
		Application icindeki stage'in iki tane sahibi var.
		1-Application
		2-Lua

		stage en son unref yapanin (dolayisiyla silinmesine neden olanin) Lua olmasi onemli.
		bu nedenle ilk once 
		1-application_->releaseView(); diyoruz (bu stage_->unref() yapiyor) sonra
		2-lua_close(L) diyoruz
	*/

//	Referenced::emptyPool();

//	SoundContainer::instance().stopAllSounds();

	application_->releaseView();

	PluginManager& pluginManager = PluginManager::instance();
	for (size_t i = 0; i < pluginManager.plugins.size(); ++i)
        pluginManager.plugins[i].main(L, 1);

	lua_close(L);
	L = NULL;

	delete application_;
    application_ = NULL;

    clearError();
//	Referenced::emptyPool();
}


#ifndef abs_index

/* convert a stack index to positive */
#define abs_index(L, i)		((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : \
	lua_gettop(L) + (i) + 1)

#endif

static int tick(lua_State *L)
{
    gevent_Tick();
    return 0;
}

static int enterFrame(lua_State* L)
{
    StackChecker checker(L, "enterFrame", 0);

    LuaApplication *luaApplication = static_cast<LuaApplication*>(luaL_getdata(L));
    Application *application = luaApplication->getApplication();

	setEnvironTable(L);

	luaL_rawgetptr(L, LUA_REGISTRYINDEX, &key_events);
	luaL_nullifytable(L, -1);
	lua_pop(L, 1);

    gevent_Tick();

    PluginManager& pluginManager = PluginManager::instance();
    for (size_t i = 0; i < pluginManager.plugins.size(); ++i)
        if (pluginManager.plugins[i].enterFrame)
            pluginManager.plugins[i].enterFrame(L);

    application->enterFrame();

	return 0;
}

void LuaApplication::tick(GStatus *status)
{
    void *pool = application_->createAutounrefPool();

    lua_pushlightuserdata(L, &key_tickFunction);
    lua_rawget(L, LUA_REGISTRYINDEX);

    if (lua_pcall_traceback(L, 0, 0, 0))
    {
        if (exceptionsEnabled_ == true)
        {
            if (status)
                *status = GStatus(1, lua_tostring(L, -1));
        }
        lua_pop(L, 1);
    }

    application_->deleteAutounrefPool(pool);
}

void LuaApplication::enterFrame(GStatus *status)
{
    void *pool = application_->createAutounrefPool();

	StackChecker checker(L, "enterFrame", 0);

    lua_pushlightuserdata(L, &key_enterFrameFunction);
	lua_rawget(L, LUA_REGISTRYINDEX);

    if (lua_pcall_traceback(L, 0, 0, 0))
	{
		if (exceptionsEnabled_ == true)
		{
            if (status)
                *status = GStatus(1, lua_tostring(L, -1));
        }
        lua_pop(L, 1);
    }

    application_->deleteAutounrefPool(pool);
}

void LuaApplication::clearBuffers()
{
	application_->clearBuffers();
}

void LuaApplication::renderScene(int deltaFrameCount)
{
	application_->renderScene();
}

void LuaApplication::setPlayerMode(bool isPlayer)
{
    isPlayer_ = isPlayer;
}


bool LuaApplication::isPlayerMode()
{
    return isPlayer_;
}

lua_PrintFunc LuaApplication::getPrintFunc(void)
{
	return printFunc_;
}

void LuaApplication::setPrintFunc(lua_PrintFunc func, void *data)
{
    printFunc_ = func;
    printData_ = data;
	if (L)
        lua_setprintfunc(L, printFunc_, printData_);
}

void LuaApplication::enableExceptions()
{
	exceptionsEnabled_ = true;
}

void LuaApplication::disableExceptions()
{
	exceptionsEnabled_ = false;
}


void LuaApplication::setHardwareOrientation(Orientation orientation)
{
	orientation_ = orientation;
	application_->setHardwareOrientation(orientation_);
}


void LuaApplication::setResolution(int width, int height)
{
	width_ = width;
	height_ = height;

	application_->setResolution(width_, height_);
}


struct Touches_p
{
	Touches_p(const std::vector<Touch*>& touches, const std::vector<Touch*>& allTouches) :
		touches(touches),
		allTouches(allTouches)
	{

	}
	
	const std::vector<Touch*>& touches;
	const std::vector<Touch*>& allTouches;
};


/*
void LuaApplication::broadcastApplicationDidFinishLaunching()
{
	Event event(Event::APPLICATION_DID_FINISH_LAUNCHING);
	broadcastEvent(&event);
}

void LuaApplication::broadcastApplicationWillTerminate()
{
	Event event(Event::APPLICATION_WILL_TERMINATE);
	broadcastEvent(&event);
}

void LuaApplication::broadcastMemoryWarning()
{
	Event event(Event::MEMORY_WARNING);
	broadcastEvent(&event);
}
*/


static int broadcastEvent(lua_State* L)
{
    Event* event = static_cast<Event*>(lua_touserdata(L, 1));
    lua_pop(L, 1);				// TODO: event system requires an empty stack, preferibly correct this silly requirement

	setEnvironTable(L);

	EventDispatcher::broadcastEvent(event);

	return 0;
}

void LuaApplication::broadcastEvent(Event* event, GStatus *status)
{
    void *pool = application_->createAutounrefPool();

	lua_pushcfunction(L, ::broadcastEvent);
	lua_pushlightuserdata(L, event);

    if (lua_pcall_traceback(L, 1, 0, 0))
	{
		if (exceptionsEnabled_ == true)
		{
            if (status)
                *status = GStatus(1, lua_tostring(L, -1));
		}
        lua_pop(L, 1);
	}

    application_->deleteAutounrefPool(pool);
}

/*
static int orientationChange(lua_State* L)
{
	Application* application = static_cast<Application*>(lua_touserdata(L, 1));
	Orientation orientation = static_cast<Orientation>(lua_tointeger(L, 2));
	lua_pop(L, 2);				// TODO: event system requires an empty stack, preferibly correct this silly requirement

	setEnvironTable(L);

	application->orientationChange(orientation);

	return 0;
}

void LuaApplication::orientationChange(Orientation orientation)
{
	lua_pushcfunction(L, ::orientationChange);
	lua_pushlightuserdata(L, application_);
	lua_pushinteger(L, orientation);

	if (lua_pcall_traceback(L, 2, 0, 0))
	{
		if (exceptionsEnabled_ == true)
		{
			LuaException exception(LuaException::eRuntimeError, lua_tostring(L, -1));
			lua_pop(L, lua_gettop(L)); // we always clean the stack when there is an error
			throw exception;
		}
		else
		{
			lua_pop(L, lua_gettop(L)); // we always clean the stack when there is an error
		}
	}
}
*/
bool LuaApplication::isInitialized() const
{
	return application_ != NULL;
}

void LuaApplication::initialize()
{
	oglReset();

    clearError();

	physicsScale_ = 30;

	application_ = new Application;

	application_->setHardwareOrientation(orientation_);
	application_->setResolution(width_, height_);
	application_->setScale(scale_);

#if defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define ARCH_X64 1
#else
#define ARCH_X64 0
#endif

    if (ARCH_X64 && lua_isjit())
        L = luaL_newstate();
    else
        L = lua_newstate(l_alloc, NULL);

    lua_pushlightuserdata(L, &key_tickFunction);
    lua_pushcfunction(L, ::tick);
    lua_rawset(L, LUA_REGISTRYINDEX);

    lua_pushlightuserdata(L, &key_enterFrameFunction);
	lua_pushcfunction(L, ::enterFrame);
	lua_rawset(L, LUA_REGISTRYINDEX);

#if 0
	if (L == NULL)
	{
		fprintf(stderr, "cannot create Lua state\n");
		return;
	}
#endif

	application_->initView();


    lua_setprintfunc(L, printFunc_, printData_);
    luaL_setdata(L, this);

	luaL_openlibs(L);

	//	lua_sethook(L, testHook, LUA_MASKLINE, 0);

	lua_pushcfunction(L, bindAll);
	lua_pushlightuserdata(L, application_);
	lua_call(L, 1, 0);
}

void LuaApplication::setScale(float scale)
{
	scale_ = scale;
	application_->setScale(scale_);
}

void LuaApplication::setLogicalDimensions(int width, int height)
{
	application_->setLogicalDimensions(width, height);
}

void LuaApplication::setLogicalScaleMode(LogicalScaleMode mode)
{
	application_->setLogicalScaleMode(mode);
}

int LuaApplication::getLogicalWidth() const
{
	return application_->getLogicalWidth();
}
int LuaApplication::getLogicalHeight() const
{
	return application_->getLogicalHeight();
}
int LuaApplication::getHardwareWidth() const
{
	return application_->getHardwareWidth();
}
int LuaApplication::getHardwareHeight() const
{
	return application_->getHardwareHeight();
}

void LuaApplication::setImageScales(const std::vector<std::pair<std::string, float> >& imageScales)
{
	application_->setImageScales(imageScales);
}

const std::vector<std::pair<std::string, float> >& LuaApplication::getImageScales() const
{
	return application_->getImageScales();
}

void LuaApplication::setOrientation(Orientation orientation)
{
	application_->setOrientation(orientation);
}

Orientation LuaApplication::orientation() const
{
	return application_->orientation();
}

void LuaApplication::addTicker(Ticker* ticker)
{
	application_->addTicker(ticker);
}

void LuaApplication::removeTicker(Ticker* ticker)
{
	application_->removeTicker(ticker);
}

float LuaApplication::getLogicalTranslateX() const
{
	return application_->getLogicalTranslateX();
}

float LuaApplication::getLogicalTranslateY() const
{
	return application_->getLogicalTranslateY();
}

float LuaApplication::getLogicalScaleX() const
{
	return application_->getLogicalScaleX();
}

float LuaApplication::getLogicalScaleY() const
{
	return application_->getLogicalScaleY();
}

void LuaApplication::setError(const char* error)
{
    error_ = error;
}

bool LuaApplication::isErrorSet() const
{
    return !error_.empty();
}

const char* LuaApplication::getError() const
{
    return error_.c_str();
}

void LuaApplication::clearError()
{
    error_.clear();
}

lua_State *LuaApplication::getLuaState() const
{
    return L;
}


