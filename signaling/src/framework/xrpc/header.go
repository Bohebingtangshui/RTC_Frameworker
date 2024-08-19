package xrpc

const (
	HEADER_SIZE     = 36
	HEADER_MAGICNUM = 32
)

type Header struct {
	Id       uint16
	Version  uint16
	LogId    uint32
	Provider [16]byte
	MagicNum uint32
	Reserved uint32
	BodyLen  uint32
}
