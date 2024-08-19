package action

import (
	"fmt"
	"html/template"
	"net/http"
	"signaling/src/framework"
)

type xrtcClientPushAction struct{}

func NewXrtcClientPushAction() *xrtcClientPushAction {
	return &xrtcClientPushAction{}
}

func writeHtmlErrorResponse(w http.ResponseWriter, status int, message string) {
	w.WriteHeader(status)
	w.Write([]byte(message))
}
func (*xrtcClientPushAction) Execute(w http.ResponseWriter, cr *framework.ComRequest) {
	r := cr.R
	err := r.ParseForm()
	if err != nil {
		fmt.Println("Error parsing data: ", err)
		writeHtmlErrorResponse(w, http.StatusBadRequest, "400 - Bad Request")
		return
	}
	fmt.Println("Raw form data:", r.Form)

	t, err := template.ParseFiles(framework.GetStaticDir() + "/template/push.tpl")
	if err != nil {
		fmt.Println("Error parsing template: ", err)
		writeHtmlErrorResponse(w, http.StatusNotFound, "404 - Not found")
		return
	}

	request := make(map[string]string)
	for k, v := range r.Form {
		request[k] = v[0]
	}
	fmt.Println("Received data:", request)

	err = t.Execute(w, request)
	if err != nil {
		fmt.Println("Error executing template: ", err)
		writeHtmlErrorResponse(w, http.StatusInternalServerError, "500 - Internal Server Error")
		return
	}

}
