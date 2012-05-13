/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
 
#include "precompiled.h"
#include <Rocket/Core/Lua/Interpreter.h>
#include <Rocket/Core/Lua/Utilities.h>
#include <Rocket/Core/Log.h>
#include <Rocket/Core/String.h>
#include <Rocket/Core/Lua/LuaType.h>
#include "LuaDocumentElementInstancer.h"
#include <Rocket/Core/Factory.h>
#include "LuaEventListenerInstancer.h"
#include "Rocket.h"
#include <ElementStyle.h>
//the types I made
#include "ContextDocumentsProxy.h"
#include "EventParametersProxy.h"
#include "ElementAttributesProxy.h"
#include "Log.h"
#include "Element.h"
#include "ElementStyle.h"
#include "Document.h"
#include "Colourb.h"
#include "Colourf.h"
#include "Vector2f.h"
#include "Vector2i.h"
#include "Context.h"
#include "Event.h"
#include "ElementInstancer.h"

namespace Rocket {
namespace Core {
namespace Lua {
lua_State* Interpreter::_L = NULL;
//typedefs for nicer Lua names
typedef Rocket::Core::ElementDocument Document;


void Interpreter::Startup()
{
    Log::Message(Log::LT_INFO, "Loading Lua interpreter");
    _L = luaL_newstate();
    luaL_openlibs(_L);

    RegisterEverything(_L);
}


void Interpreter::RegisterEverything(lua_State* L)
{
    LuaType<Vector2i>::Register(L);
    LuaType<Vector2f>::Register(L);
    LuaType<Colourf>::Register(L);
    LuaType<Colourb>::Register(L);
    LuaType<Log>::Register(L);
    LuaType<ElementStyle>::Register(L);
    LuaType<Element>::Register(L);
        //things that inherit from Element
        LuaType<Document>::Register(L);
    LuaType<Event>::Register(L);
    LuaType<Context>::Register(L);
    LuaType<rocket>::Register(L);
    LuaType<ElementInstancer>::Register(L);
    //Proxy tables
    LuaType<ContextDocumentsProxy>::Register(L);
    LuaType<EventParametersProxy>::Register(L);
    LuaType<ElementAttributesProxy>::Register(L);
}



void Interpreter::LoadFile(const String& file)
{
    String msg = "Loading";
    if(luaL_loadfile(_L, file.CString()) != 0)
    {
        msg.Append(" failed. Could not load. ").Append(file);
        Log::Message(Log::LT_ERROR, msg.CString());
        Report(_L);
    }
    else
    {
        if(lua_pcall(_L,0,0,0) != 0)
        {
            msg.Append(" failed. Could not run. ").Append(file);
            Log::Message(Log::LT_ERROR, msg.CString());
            Report(_L);
        }
        else
        {
            msg.Append(" was successful. ").Append(file);
            Log::Message(Log::LT_DEBUG, msg.CString());
        }
    }
}


void Interpreter::DoString(const Rocket::Core::String& code, const Rocket::Core::String& name)
{
    luaL_loadbuffer(_L,code.CString(),code.Length(), name.CString());
    if(lua_pcall(_L,0,0,0) != 0)
        Report(_L);
}

void Interpreter::LoadString(const Rocket::Core::String& code, const Rocket::Core::String& name)
{
    luaL_loadbuffer(_L,code.CString(),code.Length(), name.CString());
}


void Interpreter::BeginCall(int funRef)
{
    lua_settop(_L,0); //empty stack
    lua_getref(_L,funRef);
}

bool Interpreter::ExecuteCall(int params, int res)
{
    bool ret = true;
    int top = lua_gettop(_L);
    //String strtype = lua_typename(_L,top-params);
    String strtype;
    for(int i = top; i >= 1; i--)
        strtype = lua_typename(_L,lua_type(_L,i));
    if(lua_type(_L,top-params) != LUA_TFUNCTION)
    {
        ret = false;
        //stack cleanup
        if(params > 0)
        {
            for(int i = top; i >= (top-params); i--)
            {
                if(!lua_isnone(_L,i))
                    lua_remove(_L,i);
            }
        }
    }
    else
    {
        if(lua_pcall(_L,params,res,0) != 0)
        {
            Report(_L);
            ret = false;
        }
    }
    return ret;
}

void Interpreter::EndCall(int res)
{
    //stack cleanup
    for(int i = res; i > 0; i--)
    {
        if(!lua_isnone(_L,res))
            lua_remove(_L,res);
    }
}

lua_State* Interpreter::GetLuaState() { return _L; }


//From Plugin
int Interpreter::GetEventClasses()
{
    return EVT_BASIC;
}

void Interpreter::OnInitialise()
{
    Startup();
    Factory::RegisterElementInstancer("body",new LuaDocumentElementInstancer())->RemoveReference();
    Factory::RegisterEventListenerInstancer(new LuaEventListenerInstancer())->RemoveReference();
}

void Interpreter::OnShutdown()
{
	//causing crashes
    //lua_close(_L);
}

void Interpreter::Initialise()
{
    Rocket::Core::RegisterPlugin(new Interpreter());
}

void Interpreter::Shutdown()
{
	lua_close(_L);
}

}
}
}
