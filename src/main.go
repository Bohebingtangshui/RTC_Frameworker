package main

import (
	"flag"
	"signaling/src/framework"
	"signaling/src/glog"
)

func main() {
	flag.Parse()

	err := framework.Init()

	if err != nil {
		panic(err)
	}

	glog.Info("Starting HTTP server")
	glog.Flush()
	err = framework.StartHttp()
	if err != nil {
		panic(err)

	}


}
