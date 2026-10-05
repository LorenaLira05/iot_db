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

/* selo do lote: % de leituras das últimas 24 h dentro da faixa */
CREATE OR REPLACE VIEW vw_lote_saude AS
WITH lote_etapa AS (
  SELECT l.id_lote, l.fk_fruta_id_fruta, l.status,
         CASE l.status
           WHEN 'Em produção'          THEN 'campo'
           WHEN 'Pronto para colheita' THEN 'campo'
           WHEN 'Em transporte'        THEN 'transporte'
           ELSE 'armazenamento'
         END AS etapa
  FROM lote l
), pct AS (
  SELECT le.id_lote,
         COUNT(*) AS n,
         100.0 * COUNT(*) FILTER (
           WHERE r.temperatura BETWEEN c.temp_min AND c.temp_max
             AND r.umidade     BETWEEN c.umidade_min AND c.umidade_max
         ) / NULLIF(COUNT(*), 0) AS pct_na_faixa
  FROM lote_etapa le
  JOIN sensor s ON s.fk_lote_id_lote = le.id_lote
  JOIN leitura_climatica r ON r.fk_sensor_id_sensor = s.id_sensor
                          AND r.data_hora >= now() - interval '24 hours'
  JOIN config_parametro c ON c.fk_fruta_id_fruta = le.fk_fruta_id_fruta
                         AND c.etapa = le.etapa
  GROUP BY le.id_lote
)
