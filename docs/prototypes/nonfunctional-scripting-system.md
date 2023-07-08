# Scripting System

The Git branch `nonfunctional-prototype-scripting` contains the prototype Angelscript-based scripting system.

This system is intended to be used in multiplayer only. Saving and restoring custom entities and other script state is too complicated to integrate in this game engine.

> <span style="background-color:orange; color: black">Warning
> </br>
> This is an unfinished implementation and likely has bugs, stability issues, and can crash and freeze. It has not been extensively tested and has edge cases that need to be accounted for.</span>

It is not possible to copy paste this into a mod and have a working scripting system. You will need a deep understanding of Assembly, C++, Angelscript's language and library functionality and the Half-Life 1 SDK and its engine to get this to a working state.

This document refers to Sven Co-op's Angelscript implementation as "older design".

The scripting system uses two loggers: `angelscript` for low level logging and `scripting` for higher level functionality.

Table of contents:
* [General design changes](#general-design-changes)
* [Content included for testing](#content-included-for-testing)
* [Features](#features)
	* [Plugins](#plugins)
		* [Plugin configuration file syntax](#plugin-configuration-file-syntax)
			* [Example](#example)
		* [Functions called automatically by the game](#functions-called-automatically-by-the-game)
	* [Map scripts](#map-scripts)
	* [trigger_script](#trigger_script)
	* [Event System](#event-system)
		* [Creating an event type in C++](#creating-an-event-type-in-c)
		* [Publishing an event](#publishing-an-event)
		* [Subscribing to and unsubscribing from an event](#subscribing-to-and-unsubscribing-from-an-event)
	* [Scheduler](#scheduler)
	* [Calling functions from C++](#calling-functions-from-c)
	* [Entity APIs](#entity-apis)
	* [Custom Entities](#custom-entities)
		* [Getting the code for base classes](#getting-the-code-for-base-classes)
		* [Anatomy of a custom entity](#anatomy-of-a-custom-entity)
		* [Defining and registering a custom entity class](#defining-and-registering-a-custom-entity-class)
		* [A more complex example](#a-more-complex-example)
	* [User Data](#user-data)
	* [Console Commands](#console-commands)
* [Closing Thoughts](#closing-thoughts)

## General design changes

Compared to the old design this implementation favors the use of global functions and properties when an object instance is not required.

A good example of this is the function used to find entities by classname.

In the old design this is `CEntityFuncs::FindEntityByClassname`, accessed using `g_EntityFuncs` like this:
```cpp
CBaseEntity@ entity = g_EntityFuncs.FindEntityByClassname(null, "func_button");
```

In the new design this is `Entities::FindEntityByClassname`, accessed like this:
```cpp
CBaseEntity@ entity = Entities::FindEntityByClassname(null, "func_button");
```

The difference is that no `this` pointer has to be passed in which makes the new design more efficient. Because these functions are defined in a namespace it is also possible to add functions to the same namespace which allows scripts to group functionality together more easily.

On the C++ side the use of container classes like `std::string` and `std::vector` simplifies setup logic. Smart pointers help to manage object lifetime more easily and limits the possibility of memory leaks and dangling pointers.

Only one script context is created; in the old design nested calls use separate contexts which may result in the creation of many contexts. These contexts would then be destroyed soon after which may result in performance issues.

The new design uses the state pushing functionality to avoid this problem.

Only a few hard-coded function calls exist, everything else uses callbacks which eliminates the need to look up and cache functions. This reduces memory usage and simplifies the implementation.

The amount of work done at runtime to process function signatures (checking parameter and return types) and setting up callbacks has been dramatically reduced through the use of simpler methods.

The scheduler for instance now accepts a single delegate type. Script authors are now required to capture state themselves using a class which eliminates the need to do this manually in the scheduler itself.

Because it accepts a delegate an incompatible function is now a compile time error rather than a runtime error.

The same is true for the event system. it also uses delegates defined at compile time rather than accepting any input and validating it at runtime.

Function execution now relies on variadic templates which allows the compiler to optimize the call much more so than the old C-style variadic arguments was ever able to.

The old design parsed out arguments by evaluating the function being called which could cause all sorts of problems since it pulls arguments from the stack, even if the types don't match or no arguments were passed.

The new design passes arguments based on what the C++ code passes in and the Angelscript context validates inputs to make sure they are correct.

No validation is performed to ensure the correct number of arguments are passed, this is something that the programmer has to make certain of when setting up an API that accepts callbacks.

This approach is much more efficient and safer.

The event system no longer allows a callback to stop further callbacks from being executed. This behavior was based on Metamod but it caused problems in plugins and resulted in authors instructing server operators to put plugins in the configuration file in a specific order to sidestep the problem.

Last but not least there is a lot more diagnostic output available using the relevant loggers which can help to diagnose problems.

## Content included for testing

Included in the assets repository is content used to test plugins and maps scripts:
* cfgsrc/maps/test_trigger_script.json
* fgdsrc/trigger_script.fgd
* pluginsrc (all files)

Note that the plugin includes map scripts as well to test compatibility with them. This means some functionality in the map scripts won't be used since the plugin takes precedence.

Some warnings are logged to the console because of conflicts in the scripts. This is intentional to test error handling.

## Features

### Plugins

Plugins are server scripts that can be loaded through a configuration file.

Plugins have one or more root scripts associated with them. Each root script is loaded into an [Angelscript module](https://www.angelcode.com/angelscript/sdk/docs/manual/doc_module.html).

Script code is only shared between modules if it is marked as [shared](https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_shared.html).

Shared code is accessible to all modules so it should be used with care and limited to the bare minimum needed to communicate between separate scripts.

Plugins are specified in the plugin configuration file specified by the cvar `sv_script_plugin_list_file`, with the default filename being `default_plugins.json`.

#### Plugin configuration file syntax

The plugin configuration file is a list of objects.

Each object represents a plugin and contains a name and a list of root scripts.

##### Example

```jsonc
[
{
	"Name": "TestPlugin",
	"Scripts": [
		"maps/test.as",
		"maps/another.as",
		"plugins/plugin.as"
	]
}
]
```

#### Functions called automatically by the game

There are 2 functions called by the game based on name alone:
* `ScriptInit`: Called to initialize a script after it has been loaded
* `ScriptShutdown`: Called to shut down a script right before it is unloaded

All other script function execution performed by the game is done through callbacks, usually using the event system or custom entities.

These functions cannot be marked shared since each module can have its own version.

#### Map scripts

Map scripts are plugins in every way. The only distinction is that map scripts are interacted with in a slightly different way in some cases and map script plugins are managed automatically by the game.

Map scripts are specified in the map configuration file:
```jsonc
"Sections": {
	"Scripts": {
		"Scripts": [
			"maps/test.as",
			"maps/duplicate.as"
		]
	}
}
```

Note that the extra level of indirection is to allow additional data to be configured if needed later on.

### trigger_script

This entity can be used to execute a function in a map script.

The current implementation will only call the first function it finds with the given name; if a map specifies multiple root scripts and more than one has a function with the given name the rest are ignored.

This can be changed in the implementation by storing the function to execute as a list instead and executing all of them.

### Event System

The event system is used by scripts to respond to events that occur in the game.

An event is handled by subscribing to it and providing a callback. Event data is passed in using a single event object.

Event objects may provide a way to return data by setting it on the event object. No data is returned from the callback functions themselves.

It is not possible to stop the invoking of event callbacks. All callbacks are always invoked unless an error occurs during invocation.

This means it is possible for another callback to replace data being returned through an event object.

#### Creating an event type in C++

To create a new event type, create a class that inherits from `EventArgs` and register it for use in the Angelscript API:
```cpp
class SayTextEventArgs : public EventArgs
{
public:
	SayTextEventArgs(std::string&& allText, std::string&& command)
		: AllText(std::move(allText)), Command(std::move(command))
	{
	}

	const std::string AllText;
	const std::string Command;

	bool Suppress = false;
};

static void RegisterSayTextEventArgs(asIScriptEngine& engine, EventSystem& eventSystem)
{
	const char* const name = "SayTextEventArgs";

	eventSystem.RegisterEvent<SayTextEventArgs>(name);

	engine.RegisterObjectProperty(name, "const string AllText", asOFFSET(SayTextEventArgs, AllText));
	engine.RegisterObjectProperty(name, "const string Command", asOFFSET(SayTextEventArgs, Command));
	engine.RegisterObjectProperty(name, "bool Suppress", asOFFSET(SayTextEventArgs, Suppress));
}
```

#### Publishing an event

To publish an event for scripts to receive simply call `Publish`:
```cpp
// Inform plugins, see if any of them want to handle it.
const auto event = scripting::g_Scripting.GetEventSystem()->Publish<scripting::SayTextEventArgs>(p, pcmd);

if (event->Suppress)
{
	return;
}
```

The function returns the event object to allow access to values returned through the object.

#### Subscribing to and unsubscribing from an event

Each event type has a function delegate type representing the callback to use named `<EventName>Handler` and a pair of `Subscribe` and `Unsubscribe` functions to use for subscribing and unsubscribing, respectively.

These functions take a delegate of the event's type. Thus, to subscribe or unsubscribe the callback needs to be passed in:
```cpp
Events::Subscribe(@Callback);

EventCallback eventCallback;
Events::Subscribe(SayTextEventArgsHandler(@eventCallback.Callback));

void Callback(SayTextEventArgs@ foo)
{
	log::info("Callback executed");
	log::info(foo.AllText);
	log::info(foo.Command);
}

class EventCallback
{
	void Callback(SayTextEventArgs@ foo)
	{
		log::info("Object Callback executed");
		log::info(foo.AllText);
		log::info(foo.Command);
	}
}
```

Unsubscribing is done the same way:
```cpp
Events::Unsubscribe(@Callback);
Events::Unsubscribe(SayTextEventArgsHandler(@eventCallback.Callback));
```

### Scheduler

The scheduler is used to automatically execute functions after a certain amount of time has passed, optionally repeating at a specified interval.

The scheduler accepts function delegates of type `void ScheduledCallback()`.

Use classes to to include state with the callback.

```cpp
State state;
state.Data = "Hello World!";

// Execute callback every 1.5 seconds for a total of 3 times.
Scheduler.Schedule(ScheduledCallback(@state.Callback), 1.5, 3);

class State
{
	string Data;
	
	int Count = 1;

	void Callback()
	{
		log::info("Data is: " + Data + ", " + Count);
		++Count;
	}
}

```

### Calling functions from C++

Angelscript uses `IScriptContext` to manage the execution of script functions from C++.

Normally this process is quite involved and requires a lot of boilerplate code to be set up even for a simple function.

For example, to call the function `bool FInViewCone(CBaseEntity* pEntity)`:
```cpp
int result = m_Context->Prepare(function);

if (result < 0)
{
	// Handle error...
}

result = m_Context->SetObject(object);

if (result < 0)
{
	// Handle error...
}

result = m_Context->SetArgObject(0, entity);

if (result < 0)
{
	// Handle error...
}

result = m_Context->Execute();

if (result != asEXECUTION_FINISHED)
{
	// Handle error or suspended state...
}

bool isInViewCone = m_Context->GetReturnByte() != 0;
```

A lot can go wrong when calling a function and handling errors can be difficult.

To simplify all of this some helper functions are provided in `AsGenericCall.h`.

In this case you'd use this code:
```cpp
bool isInViewCone = call::ExecuteObjectMethod<bool>(*m_Context, object, function, call::Object{entity}).value_or(false);
```

The template parameter is the return type. This can be one of the following:
* `call::ReturnVoid` to for void functions
* `call::ReturnObject<type>` for objects returned by value
* `<anything else>` for primitive types, pointers, Angelscript references

The context, object and function are the same as you'd normally use.

`call::Object` is used to tell the function that the parameter is a pointer to an object that needs to be set using `SetArgObject`. Otherwise `SetArgAddress` will be used.

The returned object is a `std::optional<return_type>` and can be used to check if execution succeeded.

These helper functions handle all of the work involved, most of it done at compile time so there is very little runtime overhead added compared to manually writing out the code. Error logging is also done automatically.

Care must be taken to ensure that the function being called is compatible with the arguments being passed because there is no validation performed. Some debug-only checks are performed to warn if the wrong number of arguments are passed.

The `PushContextState` type is used to push the state of the script context. This is needed when performing nested calls, meaning if you execute a script function and that function calls a C++ function that then calls another script function you need to push the state so the context can execute that function.

Note that not all execution code in this prototype adheres to this so this needs to be double checked.

### Entity APIs

Two entity classes are accessible in scripts:
* `CBaseEntity`
* `CBaseDelay`

These show most of the work needed to register entity APIs. Properties that require conversion have wrapper functions to access them. Casts are added to convert between types at runtime.

Note that casting uses dynamic casts which means Run-Time Type Information (RTTI) must be enabled. This is normally enabled by default but some projects may disable it.

Because entities are not reference counted it is possible to for a script to store a reference to an entity that is then deleted, leaving a dangling reference behind.

A possible solution is to expose entities as a safe handle type instead, which would be a value type that stores the entity index, serial number and server spawn count to uniquely reference an entity without becoming a dangling reference even if a level change occurs.

This would require all entity APIs to use wrappers to add null checks prior to executing any functions or accessing any properties.

### Custom Entities

Scripts can define and register custom entities by inheriting from a script base class and registering the type.

A level designer can then place the entity in their map and use it.

#### Getting the code for base classes

The server command `script_write_custom_entity_baseclasses` writes all of the base classes used by the game to the file `script_custom_baseclasses.txt`.

This allows you to see which classes can be inherited from. A custom entity class must inherit from one of these to be registered.

Sample output for the prototype:
```cpp
// Script base class declarations

shared abstract class ScriptBaseClass : ScriptBaseClassInterface
{
	private CustomEntityCallbackHandler@ m_CallbackHandler;

	void SetThink(ThinkFunction@ function) final
	{
		m_CallbackHandler.SetThinkFunction(function);
	}

	void SetTouch(TouchFunction@ function) final
	{
		m_CallbackHandler.SetTouchFunction(function);
	}

	void SetBlocked(BlockedFunction@ function) final
	{
		m_CallbackHandler.SetBlockedFunction(function);
	}

	void SetUse(UseFunction@ function) final
	{
		m_CallbackHandler.SetUseFunction(function);
	}
}

shared abstract class ScriptBaseEntity : ScriptBaseClass
{
	private CBaseEntity@ m_Self = null;
	CBaseEntity@ self { get const final { return @m_Self; } }

	private BaseEntity@ m_BaseClass;
	BaseEntity@ BaseClass { get const final { return @m_BaseClass; } }
}

// External class declarations added to each plugin
external shared class ScriptBaseClass;
external shared class ScriptBaseEntity;
```

`ScriptBaseClassInterface` is a tag interface used to indicate that a class is a custom entity class but is otherwise not used. It may not be necessary at all and can be removed (holdover from older design).

`ScriptBaseClass` provides functionality shared between all custom entities. Right now that's just helper functions to set Think, Touch, Blocked and Use functions.

`ScriptBaseEntity` stores handles to the C++ entity and provides an interface to call base class implementations of virtual functions that custom entities can override to allow calling the original C++ code version if needed.

The custom entity base classes are in their own module and are accessed as shared Angelscript classes.

#### Anatomy of a custom entity

A custom entity is comprised of 3 components:
1. C++ entity: the C++ class that acts as a wrapper for the Angelscript object. Virtual functions are overridden and forward calls to the Angelscript instance if it implements the function.
2. BaseClass type: Used to call base class implementations of a virtual function. This mimics the C++ syntax used to call base class versions: `BaseClass::MyFunction();`.
3. CustomEntityCallbackHandler type: used to set Think, Touch, Blocked and Use functions. Never accessed directly, the wrappers in `ScriptBaseClass` take care of this.

For custom entities inheriting from `CBaseEntity` these components are:
1. `CBaseEntity`
2. `BaseEntity`

All 3 components are set on a custom entity by directly modifying private members during construction. This prevents custom classes from manipulating the objects pointed to by changing them in the constructor (as is possible in the older design).

#### Defining and registering a custom entity class

A simple custom entity simply requires the defining of a class that inherits from a script base class:
```cpp
class Test : ScriptBaseEntity
{
}
```

This can then be registered like so:
```cpp
CustomEntities::RegisterType("foo", "Test");
```

It is now possible to place an entity with classname `foo` in a level or create it with the `ent_create_custom` cheat.

#### A more complex example

This entity overrides some functions and sets itself up as a solid entity with the Scientist's model and size. It also sets a think function to run every 0.1 seconds and a touch function that prints the classname of the entity that touches it.
```cpp
class Test : ScriptBaseEntity
{
	void OnCreate()
	{
		BaseClass.OnCreate();
	}

	void Precache()
	{
		self.PrecacheModel("models/scientist.mdl");
	}
	
	void Spawn()
	{
		self.Precache();
		
		self.solid = SOLID_SLIDEBOX;
		self.movetype = MOVETYPE_STEP;
		self.SetModel("models/scientist.mdl");
		self.SetSize(VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
		
		SetThink(ThinkFunction(this.MyThink));
		self.nextthink = Globals.time + 0.1f;
		
		SetTouch(TouchFunction(this.MyTouch));
	}
	
	void MyThink()
	{
		log::info("Thinking at " + Globals.time);
		self.nextthink = Globals.time + 0.1f;
	}
	
	void MyTouch(CBaseEntity@ other)
	{
		log::info("Touched " + (other is null ? "unknown" : other.ClassName));
	}
}
```

### User Data

Entities have a `dictionary@ UserData` property that can be used to store off any data using a key. This allows arbitrary data to be associated with entities:
```cpp

void TriggerMe(CBaseEntity@ activator, CBaseEntity@ caller, USE_TYPE useType, float value)
{
	// Count activation times based on activator.
	dictionary@ data = activator.UserData;
	
	int count = 0;
	data.get("count", count);
	
	++count;
	
	data.set("count", count);
	
	log::info("I've been triggered " + count + " times");
}
```

### Console Commands

Scripts can create console commands using a Source-like approach:
```cpp
ConCommand MyCommand("my_script_command", @MyCommandCallback);

void MyCommandCallback(CommandArgs@ args)
{
	log::info("Command callback: " + args.Count() + " arguments");
	
	for (int i = 0; i < args.Count(); ++i)
	{
		log::info("Argument " + i + ": " + args.Argument(i));
	}
}

ConVar MyOtherVar("my_script_var", "10", @MyVarCallback);

void MyVarCallback(ConVar@ var)
{
	log::info("Variable changed to " + var.String);
}

ConVar MyVar("my_other_script_var", "foo");
```

This is only a bare-bones implementation. For a more Sven Co-op-like system additional features will be needed like namespacing commands based on which plugin they are added to.

## Closing Thoughts

This prototype implements the most complex parts of a scripting system that works like Sven Co-op's. Most of the rest involves exposing entity classes and building systems to handle specific tasks like creating text-based menus for player input.

Make sure to evaluate other scripting languages first and choose the right one for your needs. Angelscript is a powerful and complex language which makes integrating it into a C++ program complicated as well.

Games that have scripting support, like Garry's Mod and Half-Life 2 (depending on the engine and game) typically opt to use a simpler language like Lua or Squirrel because it simplifies the integration and avoids edge cases.

Angelscript allows content creators to store data in a lot of places and shape it in many ways (classes, arrays, dictionaries, nested structures, etc) which makes it difficult to account for everything.

Singleplayer integration is virtually impossible to accomplish with such complexity so it is advised to choose a simpler language if the intention is to support scripting in a singleplayer or otherwise persistent environment.

Make sure to spend time figuring out what use cases there are for the scripting system. Finding a problem that can't be solved with the chosen language after your scripting system has been implemented and in use for some time can kill your entire project.

A few weeks of brainstorming and discussion can save years of work down the line so make sure to discuss it at length. Ask for feedback and opinions from the public (both from your users, other programmers and anybody who can give useful feedback) to get a good idea of what challenges await you.
