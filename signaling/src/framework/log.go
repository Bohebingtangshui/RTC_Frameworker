package framework
import (
	"signaling/src/glog"
	"math/rand"
	"time"
	"fmt"

)

func init(){
	rand.Seed(time.Now().UnixNano())
}

func GetLogId32() uint32{
	return rand.Uint32()
}

type logItem struct {
	field string
	value string
}


type timeItem struct {
	field string
	beginTime int64
	endTime int64
}
type ComLog struct {
	mainLog []logItem
	timeLog []timeItem
}

func (l *ComLog) AddNotice(field string,value string){
	l.mainLog = append(l.mainLog,logItem{field:field,value:value})
}

func (l *ComLog) TimeBegin(field string){
	l.timeLog = append(l.timeLog,timeItem{field:field,beginTime:time.Now().UnixNano()/1000})
}

func (l *ComLog) TimeEnd(field string){
	for k,v := range l.timeLog{
		if v.field == field{
			l.timeLog[k].endTime = time.Now().UnixNano()/1000
			break
		}
	}

}
func (l *ComLog) GetPrefixLog() string{
	prefixLog:=""
	for _,item := range l.mainLog{
		prefixLog += fmt.Sprintf("%s[%s]",item.field,item.value)
	}

	for _,timeitem := range l.timeLog{
		diffTime:=timeitem.endTime-timeitem.beginTime
		if diffTime<0{
			continue
		}
		fdiffTime:=float64(diffTime)/1000.0
		prefixLog += fmt.Sprintf("%s[%.3fms] ",timeitem.field,fdiffTime)
	}
	return prefixLog
}
func (c *ComLog) Debugf(format string,args ...interface{}) {
	totalLog:=c.GetPrefixLog()+format
	glog.Debugf(totalLog,args...)
}

func (c *ComLog) Infof(format string,args ...interface{}) {
	totalLog:=c.GetPrefixLog()+format
	glog.Infof(totalLog,args...)
}

func (c *ComLog) Warningf(format string,args ...interface{}) {
	totalLog:=c.GetPrefixLog()+format
	glog.Warningf(totalLog,args...)
}