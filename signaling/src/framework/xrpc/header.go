package xrpc

import (
	"encoding/binary"
	"errors"
	"io"
)

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

func (h *Header) Marshal(buf []byte) error {
	if len(buf) < HEADER_SIZE {
		return errors.New("no enough buffer for header")
	}

	binary.LittleEndian.PutUint16(buf[0:2], h.Id)
	binary.LittleEndian.PutUint16(buf[2:4], h.Version)
	binary.LittleEndian.PutUint32(buf[4:8], h.LogId)
	copy(buf[8:24], h.Provider[:])
	binary.LittleEndian.PutUint32(buf[24:28], h.MagicNum)
	binary.LittleEndian.PutUint32(buf[28:32], h.Reserved)
	binary.LittleEndian.PutUint32(buf[32:36], h.BodyLen)

	return nil
}

func (h *Header) Write(w io.Writer) (int, error) {
	var buf [HEADER_SIZE]byte
	if err := h.Marshal(buf[:]); err != nil {
		return 0, err
	}

	return w.Write(buf[:])
}
