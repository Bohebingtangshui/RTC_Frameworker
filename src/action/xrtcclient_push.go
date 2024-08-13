package action

import (
	"fmt"
	"html/template"
	"net/http"
)
type xrtcClientPushAction struct {}

func NewXrtcClientPushAction() *xrtcClientPushAction {
	return &xrtcClientPushAction{}
}


func writeHtmlErrorResponse(w http.ResponseWriter, status int, message string) {
	w.WriteHeader(status)
	w.Write([]byte(message))
}
func (*xrtcClientPushAction) Execute(w http.ResponseWriter, r *http.Request) {
	fmt.Println("Request method:", r.Method) // 打印请求方法
    fmt.Println("Request URL path:", r.URL.Path) // 打印请求路径

	err:=r.ParseForm()
	if err!=nil {
		fmt.Println("Error parsing data: ", err)
		writeHtmlErrorResponse(w, http.StatusBadRequest, "400 - Bad Request")
		return
	}
	fmt.Println("Raw form data:", r.Form)

	request := make(map[string]string)
	for k,v:=range r.Form {
		request[k]=v[0]
	}
	fmt.Println("Received data:", request)

	t,err:=template.ParseFiles("./static/template/push.tpl")
	if err!=nil {
		fmt.Println("Error parsing template: ", err)
		writeHtmlErrorResponse(w, http.StatusNotFound, "404 - Not found")
		return
	}


	err = t.Execute(w, request)
	if err!=nil {
		fmt.Println("Error executing template: ", err)
		writeHtmlErrorResponse(w, http.StatusInternalServerError, "500 - Internal Server Error")
		return
	}




}