package main

import (
	"signaling/src/framework"
)

func main() {
	err := framework.StartHttp()
	if err != nil {
		panic(err)

	}
}
