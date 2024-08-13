package framework

import (
	"fmt"
	"net/http"
)

func init() {
	http.HandleFunc("/", entry)

}

type AcitonInterface interface {
	Execute(w http.ResponseWriter, r *http.Request)
}

var GActionRouter map[string]AcitonInterface = make(map[string]AcitonInterface)

func responseError(w http.ResponseWriter, r *http.Request, status int, message string) {
	w.WriteHeader(status)
	w.Write([]byte(fmt.Sprintf("%d - %s", status, message)))
}

func entry(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path == "/favicon.ico" {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte(""))
	}
	fmt.Println("====================================", r.URL.Path)

	if action, ok := GActionRouter[r.URL.Path]; ok {
		r.ParseForm()
		if action != nil {
			action.Execute(w, r)
		}else {
			responseError(w,r,http.StatusInternalServerError, "Internal Server Error")
		}
	} else {
		responseError(w,r,http.StatusNotFound, "not found")
	}
}

func StartHttp() error {
	fmt.Println(("Starting HTTPS server..."))
	return http.ListenAndServeTLS(":443", "conf/original.pem", "conf/privatekey.pem", nil)
}
