package framework
import (
	"signaling/src/glog"
	"math/rand"
	"time"

)

func init(){
	rand.Seed(time.Now().UnixNano())
}

func GetLogId32() uint32{
	return rand.Uint32()
}
type ComLog struct {
}


func (c *ComLog) Debugf(format string,args ...interface{}) {
	glog.Debugf(format,args...)
}

func (c *ComLog) Infof(format string,args ...interface{}) {
	glog.Infof(format,args...)
}

func (c *ComLog) Warningf(format string,args ...interface{}) {
	glog.Warningf(format,args...)
}