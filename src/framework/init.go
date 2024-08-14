package framework

import (
	"fmt"
	"signaling/src/glog"
)

var gconf *FrameworkConf

func Init(confFile string) error {
	// This is a placeholder for the init function that is called when the package is imported.
	var err error
	gconf, err = loadConf(confFile)
	if err != nil {
		return err
	}
	fmt.Printf("%+v\n", gconf)
	glog.SetLogDir(gconf.logDir)
	glog.SetLogFileName(gconf.logFile)
	glog.SetLogLevel(gconf.logLevel)
	glog.SetLogToStderr(gconf.logToStderr)

	return nil

}
