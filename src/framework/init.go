package framework
import (
	"signaling/src/glog"
)
func Init() error{
	// This is a placeholder for the init function that is called when the package is imported.

	glog.SetLogDir("./log")
	glog.SetLogFileName("signaling")
	glog.SetLogLevel("DEBUG")
	glog.SetLogToStderr(true)

	return nil

}