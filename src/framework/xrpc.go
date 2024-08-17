package framework

import (
	"fmt"
	"signaling/src/framework/xrpc"
	"strings"
)

var xrpcClients map[string]*xrpc.Client = make(map[string]*xrpc.Client)

func loadXrpc() error {
	sections := configFile.GetSectionList()

	for _, section := range sections {
		if !strings.HasPrefix(section, "xrpc.") {
			continue
		}

		mSection, err := configFile.GetSection()
		if err != nil {
			return err
		}

		
	}

	return nil
}

func Call(serviceName string, req interface{}, resp interface{}, logId uint32) error {
	fmt.Println("call service:", serviceName)
	return nil
}
