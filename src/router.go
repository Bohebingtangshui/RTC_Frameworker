package main
import (
	"signaling/src/framework"
	"signaling/src/action"
)
func init() {
	framework.GActionRouter["/xrtcclient"] = action.NewXrtcClientPushAction()
}

