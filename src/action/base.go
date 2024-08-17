package action

import (
	"bytes"
	"encoding/json"
	"net/http"
	"signaling/src/comerrors"
	"signaling/src/framework"
	"strconv"
)

const (
	CMDNO_PUSH      = 1
	CMDNO_PULL      = 2
	CMDNO_ANSWER    = 3
	CMDNO_STOP_PUSH = 4
	CMDNO_STOP_PULL = 5
)

type comHttpResp struct {
	ErrorNo  int         `json:"errno"`
	ErrorMsg string      `json:"error"`
	Data     interface{} `json:"data"`
}

func writeJsonErrorResponse(cerr *comerrors.ComError, w http.ResponseWriter, cr *framework.ComRequest) {
	cr.Logger.AddNotice("errNo", strconv.Itoa(cerr.GetErrNum()))
	cr.Logger.AddNotice("errMsg", cerr.GetErrMsg())
	cr.Logger.Warningf("rerquest process failed")

	resp := comHttpResp{ErrorNo: cerr.GetErrNum(), ErrorMsg: "process error"}

	buffer := new(bytes.Buffer)
	json.NewEncoder(buffer).Encode(resp)
	w.Write(buffer.Bytes())

}
