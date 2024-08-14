package framework

import (
	"signaling/src/goconfig"
)

type FrameworkConf struct {
	logDir      string
	logFile     string
	logLevel    string
	logToStderr bool

	httpPort      int
	httpStaticDir string
}

var configFile *goconfig.ConfigFile

func loadConf(confFile string) (*FrameworkConf, error) {
	var err error

	configFile, err = goconfig.LoadConfigFile(confFile)

	if err != nil {
		return nil, err
	}

	conf := &FrameworkConf{}

	conf.logDir, err = configFile.GetValue("log", "logDir")
	if err != nil {
		return nil, err
	}

	conf.logFile, err = configFile.GetValue("log", "logFile")
	if err != nil {
		return nil, err
	}

	conf.logLevel, err = configFile.GetValue("log", "logLevel")
	if err != nil {
		return nil, err
	}

	conf.logToStderr, err = configFile.Bool("log", "logToStderr")
	if err != nil {
		return nil, err
	}

	conf.httpPort, err = configFile.Int("http", "httpPort")
	if err != nil {
		return nil, err
	}

	conf.httpStaticDir, err = configFile.GetValue("http", "httpStaticDir")
	if err != nil {
		return nil, err
	}

	return conf, nil

}

func GetStaticDir() string {
	return gconf.httpStaticDir
}
