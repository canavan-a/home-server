package database_test

import (
	"fmt"
	"main/database"
	"testing"
	"time"

	"gorm.io/driver/postgres"
	"gorm.io/gorm"
)

var CONNSTR = "postgresql://postgres:postgres@192.168.1.162:5432/house"

func Connnect() (db *gorm.DB, err error) {

	db, err = gorm.Open(postgres.Open(CONNSTR), &gorm.Config{})
	if err != nil {
		fmt.Println("Failed to connect to the database:", err)
		return
	}

	return
}

func TestHydrometerQuery(t *testing.T) {
	db, err := Connnect()
	if err != nil {
		t.Error("could not connect")
	}
	readings, err := database.GetSpacedReadings(db, 0, 100, time.Now().Add(-time.Hour))
	if err != nil {
		t.Error("bad query")
	}

	if len(readings) != 100 {
		t.Error("failed to query")
	}
}

func TestClipQuery(t *testing.T) {
	db, err := Connnect()
	if err != nil {
		t.Error("could not connect")
	}
	clips, err := database.ListClips(db)
	if err != nil {
		t.Error("bad query")
		return
	}

	if len(clips) == 0 {
		t.Error("failed to query")
		return
	}

	fmt.Println(clips)
}

func TestInsertClipQuery(t *testing.T) {
	db, err := Connnect()
	if err != nil {
		t.Error("could not connect")
	}

	err = database.InsertClip(db, database.Clip{
		Timestamp: time.Now(),
		Clip:      []byte("hello world"),
	})
	if err != nil {
		t.Error("bad query")
		return
	}
	db.Commit()

}
