use core::{fmt, marker::PhantomData};
use serde::{
    de::{self, MapAccess, Visitor},
    Deserialize, Deserializer, Serialize,
};

const MAX_LEN_OF_PATH: usize = 128;

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
#[repr(usize)]
pub enum Mode {
    Machine = 3,
    Supervisor = 1,
    User = 0,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct NextStageConfig {
    pub path: Option<heapless::String<MAX_LEN_OF_PATH>>,
    pub mode: Mode,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Config {
    pub firmware: Option<heapless::String<MAX_LEN_OF_PATH>>,
    pub opaque: Option<heapless::String<MAX_LEN_OF_PATH>>,
    pub bootargs: Option<heapless::String<MAX_LEN_OF_PATH>>,
    #[serde(deserialize_with = "deserialize_next_stage")]
    pub next_stage: NextStageConfig,
}

impl Default for Config {
    #[inline]
    fn default() -> Self {
        Config {
            firmware: Some(heapless::String::try_from("rustsbi.bin").unwrap()),
            opaque: None,
            bootargs: None,
            next_stage: NextStageConfig {
                path: None,
                mode: Mode::Supervisor,
            },
        }
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Root {
    configs: Config,
}

pub fn parse_config(data: &[u8]) -> Result<Config, picotoml::Error> {
    let toml_str = core::str::from_utf8(data).unwrap_or("");
    let root: Root = picotoml::from_str(toml_str)?;
    Ok(root.configs)
}

fn deserialize_next_stage<'de, D>(deserializer: D) -> Result<NextStageConfig, D::Error>
where
    D: Deserializer<'de>,
{
    struct StringOrStruct(PhantomData<fn() -> NextStageConfig>);

    impl<'de> Visitor<'de> for StringOrStruct {
        type Value = NextStageConfig;

        fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
            formatter.write_str("string or map")
        }

        fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
        where
            E: de::Error,
        {
            let mut path = heapless::String::new();
            let _ = path.push_str(&v[..MAX_LEN_OF_PATH]);
            let value = NextStageConfig {
                path: Some(path),
                mode: Mode::Supervisor,
            };
            Ok(value)
        }

        fn visit_map<M>(self, map: M) -> Result<NextStageConfig, M::Error>
        where
            M: MapAccess<'de>,
        {
            Deserialize::deserialize(de::value::MapAccessDeserializer::new(map))
        }
    }

    deserializer.deserialize_any(StringOrStruct(PhantomData))
}
