# User message dispatcher

The Half-Life Unified SDK changes how user messages are dispatched.

In the original SDK messages are static functions invoked directly by the engine. This has been changed to allow non-static member functions to be used.

Additionally the functionality to read messages has been redesigned to use a buffer object instead of global variables which eliminates the possibility of message reading functions being called at the wrong time.

## Registering a function

To register a function, call `g_ClientUserMessages.RegisterHandler` with the name of the message, the function to call and the object it is to be called on.

```cpp
g_ClientUserMessages.RegisterHandler("EntityInfo", &CHudEntityInfo::MsgFunc_EntityInfo, this);
```

The function itself should have one of these signatures:
```cpp
void MsgFunc_Name(const char* pszName, BufferReader& reader);
void MsgFunc_Name(BufferReader& reader);
```

It is also possible to register a static function if needed.

## Reading from the buffer

The `BufferReader` class provides functions to read data from the message. The `Read*` functions read data that was written on the server side using the `WRITE_*` macros with matching names.

It is not necessary to read all data out of the buffer. The engine always reads the full contents of the message from the incoming network buffer, so the engine's read position is not affected by read operations in the client library.

Note that when reading strings you should store or use the result of `ReadString()` immediately. The returned string is stored in a buffer that is reused every time `ReadString` is called so the string will be overwritten.

### Example

Writing a message:
```cpp
// Variables initialized earlier on
CBaseEntity* entity = ...;
RGB24 color = ...;
int roundedHealth = ...;

MESSAGE_BEGIN(MSG_ONE, gmsgEntityInfo, nullptr, this);
WRITE_STRING(STRING(entity->pev->classname));
WRITE_LONG(roundedHealth);
WRITE_RGB24(color);
MESSAGE_END();
```

Reading a message:
```cpp
void CHudEntityInfo::MsgFunc_EntityInfo(const char* pszName, BufferReader& reader)
{
	m_EntityInfo = {};

	m_EntityInfo.Classname = reader.ReadString();

	// Nothing to draw, skip rest of message.
	if (m_EntityInfo.Classname.empty())
	{
		return;
	}

	m_EntityInfo.Health = reader.ReadLong();
	m_EntityInfo.Color = reader.ReadRGB24();

	m_DrawEndTime = gHUD.m_flTime + EntityInfoDrawTime;
}
```