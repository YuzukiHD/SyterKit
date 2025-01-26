pub struct TimeSource {
    _private: (),
}

impl embedded_sdmmc::TimeSource for TimeSource {
    fn get_timestamp(&self) -> embedded_sdmmc::Timestamp {
        // TODO implement allwinner sunxi RTC based time source
        embedded_sdmmc::Timestamp::from_calendar(2023, 1, 1, 0, 0, 0).unwrap()
    }
}

pub fn time_source() -> TimeSource {
    TimeSource { _private: () }
}
