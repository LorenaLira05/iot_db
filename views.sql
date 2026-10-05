-- views.sql
-- Rodar depois dos triggers (vw_lote_saude usa fn_etapa_lote).

-- View para o dashboard
CREATE OR REPLACE VIEW vw_status_sensores AS
SELECT s.id_sensor, s.nome, s.channel_id, s.status,
       s.fk_lote_id_lote AS id_lote,
       u.data_hora AS ultima_leitura, u.temperatura, u.umidade
FROM sensor s
LEFT JOIN LATERAL (
    SELECT l.data_hora, l.temperatura, l.umidade
    FROM leitura_climatica l
    WHERE l.fk_sensor_id_sensor = s.id_sensor
    ORDER BY l.data_hora DESC
    LIMIT 1
) u ON TRUE;

-- Analista: agregações diárias por lote
CREATE OR REPLACE VIEW vw_leituras_diarias_lote AS
SELECT s.fk_lote_id_lote AS id_lote,
       l.data_hora::date AS dia,
       avg(l.temperatura) AS temp_media,
       min(l.temperatura) AS temp_min,
       max(l.temperatura) AS temp_max,
       avg(l.umidade)     AS umid_media,
       min(l.umidade)     AS umid_min,
       max(l.umidade)     AS umid_max,
       count(*)           AS qtd_leituras
FROM leitura_climatica l
JOIN sensor s ON s.id_sensor = l.fk_sensor_id_sensor
GROUP BY s.fk_lote_id_lote, l.data_hora::date;

-- Gráficos do dashboard: médias por hora (troque 'hour' por 'day' para diário)
CREATE OR REPLACE VIEW vw_leitura_hora AS
SELECT fk_sensor_id_sensor,
       date_trunc('hour', data_hora) AS hora,
       ROUND(AVG(temperatura)::numeric, 2) AS temperatura_media,
       ROUND(AVG(umidade), 2)              AS umidade_media,
       COUNT(*)                            AS n_leituras
FROM leitura_climatica
GROUP BY fk_sensor_id_sensor, date_trunc('hour', data_hora);

-- Selo do lote: % de leituras das últimas 24 h dentro da faixa da etapa
CREATE OR REPLACE VIEW vw_lote_saude AS
WITH pct AS (
  SELECT l.id_lote,
         COUNT(*) AS n,
         100.0 * COUNT(*) FILTER (
           WHERE r.temperatura BETWEEN c.temp_min AND c.temp_max
             AND r.umidade     BETWEEN c.umidade_min AND c.umidade_max
         ) / NULLIF(COUNT(*), 0) AS pct_na_faixa
  FROM lote l
  JOIN sensor s ON s.fk_lote_id_lote = l.id_lote
  JOIN leitura_climatica r ON r.fk_sensor_id_sensor = s.id_sensor
                          AND r.data_hora >= now() - interval '24 hours'
  JOIN LATERAL (
    SELECT c.* FROM config_parametro c
    WHERE c.fk_fruta_id_fruta = l.fk_fruta_id_fruta
      AND (c.etapa = fn_etapa_lote(l.status)
           OR (fn_etapa_lote(l.status) = 'transporte' AND c.etapa = 'armazenamento'))
    ORDER BY (c.etapa = fn_etapa_lote(l.status)) DESC
    LIMIT 1
  ) c ON TRUE
  GROUP BY l.id_lote
)
SELECT id_lote, n AS n_leituras, ROUND(pct_na_faixa, 1) AS pct_na_faixa,
       CASE WHEN pct_na_faixa >= 90 THEN 'Saudável'   -- limites provisórios: o grupo define
            WHEN pct_na_faixa >= 70 THEN 'Atenção'
            ELSE 'Crítico' END AS selo
FROM pct;
