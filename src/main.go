package main

import (
	"signaling/src/framework"
)

func main() {
	err := framework.Init()

	if err != nil {
		panic(err)
	}
	err = framework.StartHttp()
	if err != nil {
		panic(err)

	}
}
