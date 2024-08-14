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

	err = framework.StartHttp()
	if err != nil {
		panic(err)

	}

}
