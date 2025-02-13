package hydrometer

import (
	"fmt"
	"os"

	"gorm.io/driver/postgres"
	"gorm.io/gorm"
)

type Reading struct {
	ID        uint   `gorm:"primaryKey"`
	PlantID   int    `gorm:"not null"`
	Timestamp string `gorm:"not null"`
	Value     int    `gorm:"not null"`
}

func connect() (db *gorm.DB, err error) {
	connectionString := os.Getenv("PG_CONN_STR")

	db, err = gorm.Open(postgres.Open(connectionString), &gorm.Config{})
	if err != nil {
		fmt.Println("Failed to connect to the database:", err)
		return
	}

	return
}
