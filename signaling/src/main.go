package main

import (
	"flag"
	"signaling/src/framework"
)

func main() {
	flag.Parse()

	err := framework.Init("./conf/framework.conf")

	if err != nil {
		panic(err)
	}
	framework.RegisterStaticUrl()

	go StartHttp()

	StartHttps()

}

func StartHttp() {
	error := framework.StartHttp()
	if error != nil {
		panic(error)
	}
}

func StartHttps() {
	error := framework.StartHttps()
	if error != nil {
		panic(error)
	}
}
