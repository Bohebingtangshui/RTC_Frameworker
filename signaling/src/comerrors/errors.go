package comerrors

const (
	NoErr      = 0
	ParamErr   = 1
	NetworkErr = 2
)

type ComError struct {
	mErrNum int
	mErrMsg string
}

func NewComError(errNum int, errMsg string) *ComError {
	return &ComError{mErrNum: errNum, mErrMsg: errMsg}
}

func (ce *ComError) Errno() int {
	return ce.mErrNum
}

func (ce *ComError) Error() string {
	return ce.mErrMsg
}
