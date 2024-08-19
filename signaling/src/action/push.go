package action

import (
	"net/http"
	"signaling/src/comerrors"
	"signaling/src/framework"
	"strconv"
)

type pushAction struct{}

func NewPushAction() *pushAction {
	return &pushAction{}
}

type xrtcPushReq struct {
	Cmdno      int    `json:"cmdno"`
	Uid        uint64 `json:"uid"`
	StreamName string `json:"streamName"`
	Audio      int    `json:"audio"`
	Video      int    `json:"video"`
}

type xrtcPushResp struct {
	Errno  int    `json:"err_no"`
	ErrMsg string `json:"err_msg"`
	Offer  string `json:"offer"`
}

func (*pushAction) Execute(w http.ResponseWriter, cr *framework.ComRequest) {
	// TODO
	r := cr.R

	// get uid
	var strUid string
	if values, ok := r.Form["uid"]; ok {
		strUid = values[0]
	}
	uid, err := strconv.ParseUint(strUid, 10, 64)
	if err != nil || uid <= 0 {
		cerr := comerrors.NewComError(comerrors.ParamErr, "parse uid error:"+err.Error())
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

	// get streamName
	var streamName string
	if values, ok := r.Form["streamName"]; ok {
		streamName = values[0]
	}
	if streamName == "" {
		cerr := comerrors.NewComError(comerrors.ParamErr, "streamName is empty")
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

	// get audio video
	var strAuduio, strVideo string
	var audio, video int

	if values, ok := r.Form["audio"]; ok {
		strAuduio = values[0]
	}
	if strAuduio == "" || strAuduio == "0" {
		audio = 0
	} else {
		audio = 1
	}

	if values, ok := r.Form["video"]; ok {
		strVideo = values[0]
	}
	if strVideo == "" || strVideo == "0" {
		video = 0
	} else {
		video = 1
	}

	req := xrtcPushReq{
		Cmdno:      CMDNO_PUSH,
		Uid:        uid,
		StreamName: streamName,
		Audio:      audio,
		Video:      video,
	}
	var resp xrtcPushResp
	err = framework.Call("xrtc", req, resp, cr.LogId)
	if err != nil {
		cerr := comerrors.NewComError(comerrors.NetworkErr, "backend process error"+err.Error())
		writeJsonErrorResponse(cerr, w, cr)
		return
	}

}
