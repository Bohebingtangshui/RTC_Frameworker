package framework

import (
	"fmt"
	"signaling/src/comerrors"
	"signaling/src/framework/xrpc"
	"strconv"
	"strings"
	"time"
)

var xrpcClients map[string]*xrpc.Client = make(map[string]*xrpc.Client)

func loadXrpc() error {
	sections := configFile.GetSectionList()

	for _, section := range sections {
		if !strings.HasPrefix(section, "xrpc.") {
			continue
		}

		mSection, err := configFile.GetSection(section)
		if err != nil {
			return err
		}

		values, ok := mSection["server"]
		if !ok {
			return comerrors.NewComError(1, "no server field in config file")
		}

		arrServer := strings.Split(values, ",")
		for i, server := range arrServer {
			arrServer[i] = strings.TrimSpace(server)
		}

		client := xrpc.NewClient(arrServer)

		if values, ok = mSection["connectTimeout"]; ok {
			if connectTimeout, err := strconv.Atoi(values); err == nil {
				client.ConnectTimeout = time.Duration(connectTimeout) * time.Millisecond
			}
		}

		if values, ok = mSection["readTimeout"]; ok {
			if readTimeout, err := strconv.Atoi(values); err == nil {
				client.ReadTimeout = time.Duration(readTimeout) * time.Millisecond
			}
		}

		if values, ok = mSection["writeTimeout"]; ok {
			if writeTimeout, err := strconv.Atoi(values); err == nil {
				client.WriteTimeout = time.Duration(writeTimeout) * time.Millisecond
			}
		}

		xrpcClients[section] = client

		fmt.Println("arrServer:", client)

	}

	return nil
}

func Call(serviceName string, req interface{}, resp interface{}, logId uint32) error {

	fmt.Println("call service:", serviceName)
	return nil
}
