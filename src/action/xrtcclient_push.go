package action

import (
	"fmt"
	"net/http"
)
type xrtcClientPushAction struct {}

func NewXrtcClientPushAction() *xrtcClientPushAction {
	return &xrtcClientPushAction{}
}

func (*xrtcClientPushAction) Execute(w http.ResponseWriter, r *http.Request) {
	fmt.Println("hello xrtcClientPushAction")
	w.WriteHeader(http.StatusOK)
	w.Write([]byte("xrtcClientPushAction"))
}