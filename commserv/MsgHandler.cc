#include <assert.h>
#include "MsgHandler.h"

MsgHandler::MsgHandler()
	: func_ ()
	, threadPoolId_ (-1)
{

}

MsgHandler::MsgHandler(const MsgFunc& func, int threadPoolId)
	: func_ (func)
	, threadPoolId_ (threadPoolId)
{
}
