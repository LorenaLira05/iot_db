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
