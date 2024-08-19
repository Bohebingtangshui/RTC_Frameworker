package framework

import (
	"fmt"
	"net/http"
	"os"
	"signaling/src/glog"
	"strconv"
)

func init() {
	http.HandleFunc("/", entry)
}

type AcitonInterface interface {
	Execute(w http.ResponseWriter, cr *ComRequest)
}

var GActionRouter map[string]AcitonInterface = make(map[string]AcitonInterface)

type ComRequest struct {
	R      *http.Request
	Logger *ComLog
	LogId  uint32
}

func responseError(w http.ResponseWriter, r *http.Request, status int, message string) {
	w.WriteHeader(status)
	w.Write([]byte(fmt.Sprintf("%d - %s", status, message)))
}

func getRealClientIP(r *http.Request) string {
	clientIP := r.RemoteAddr
	if realIP := r.Header.Get("X-Real-IP"); realIP != "" {
		clientIP = realIP
	} else if realIP := r.Header.Get("X-Forwarded-For"); realIP != "" {
		clientIP = realIP
	}
	return clientIP
}

func entry(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path == "/favicon.ico" {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte(""))
		return
	}
	fmt.Println("====================================", r.URL.Path)

	if action, ok := GActionRouter[r.URL.Path]; ok {
		if action != nil {
			cr := &ComRequest{R: r, Logger: &ComLog{}, LogId: GetLogId32()}
			cr.Logger.AddNotice("logId", strconv.Itoa(int(cr.LogId)))
			cr.Logger.AddNotice("url", r.URL.Path)
			cr.Logger.AddNotice("referer", r.Header.Get("Referer"))
			cr.Logger.AddNotice("cookie", r.Header.Get("cookie"))
			cr.Logger.AddNotice("user-agent", r.Header.Get("User-Agent"))
			cr.Logger.AddNotice("clientIP", r.RemoteAddr)
			cr.Logger.AddNotice("realClientIP", getRealClientIP(r))

			r.ParseForm()

			for k, v := range r.Form {
				cr.Logger.AddNotice(k, v[0])
			}

			cr.Logger.TimeBegin("totalCost")
			action.Execute(w, cr)
			cr.Logger.TimeEnd("totalCost")

			cr.Logger.Infof("")
		} else {
			responseError(w, r, http.StatusInternalServerError, "Internal Server Error")
		}
	} else {
		responseError(w, r, http.StatusNotFound, "not found")
	}
}

func RegisterStaticUrl() {
	info, err := os.Stat(gconf.httpStaticDir)
	if os.IsNotExist(err) {
		glog.Infof("Static directory does not exist: %s", gconf.httpStaticDir)
		return
	} else if err != nil {
		glog.Infof("Failed to access static directory: %s, error: %v", gconf.httpStaticDir, err)
		return
	}

	if !info.IsDir() {
		glog.Infof("Static path is not a directory: %s", gconf.httpStaticDir)
		return
	}

	fs := http.FileServer(http.Dir(gconf.httpStaticDir))
	glog.Infof("Register static url %s", gconf.httpStaticPrefix)
	glog.Infof("Registering static url at prefix %s", gconf.httpStaticPrefix)
	http.Handle(gconf.httpStaticPrefix, http.StripPrefix(gconf.httpStaticPrefix, fs))
}

func StartHttp() error {
	glog.Infof("Start http server on port %d", gconf.httpPort)
	return http.ListenAndServe(fmt.Sprintf(":%d", gconf.httpPort), nil)
}

func StartHttps() error {
	glog.Infof("Start https server on port %d", gconf.httpsPort)
	return http.ListenAndServeTLS(fmt.Sprintf(":%d", gconf.httpsPort), gconf.httpsCert, gconf.httpsKey, nil)
}
