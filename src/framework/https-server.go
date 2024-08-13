package framework

import (
	"fmt"
	"net/http"
)

func init() {
	http.HandleFunc("/", entry)

}

type AcitonInterface interface {
	Execute(w http.ResponseWriter, cr *ComRequest)
}

var GActionRouter map[string]AcitonInterface = make(map[string]AcitonInterface)


type ComRequest struct {
	R *http.Request
	Logger *ComLog
	LogId uint32
}
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
		if action != nil {
			cr:=&ComRequest{R:r, Logger:&ComLog{}, LogId:GetLogId32()}
			r.ParseForm()
			action.Execute(w, cr)

			cr.Logger.Infof("")
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
