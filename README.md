# Banco de Dados RAIZ

Banco de dados relacional (PostgreSQL) desenvolvido para o projeto **RAIZ**, focado no monitoramento climático em tempo real, acompanhamento de lotes agrícolas, automação de alertas de contorno de faixa ideal e suporte a análises e previsões de mercado e colheita.

---

## Status do projeto

> **Em desenvolvimento**

A estrutura do banco de dados foi expandida com **restrições de integridade**, **índices de desempenho**, **gatilhos (triggers) para automação de alertas** e **views otimizadas** para dashboards e relatórios analíticos.

---

## Objetivo

Fornecer uma base de dados consistente e segura para:

* **Gestão de Acesso (RBAC):** Autenticação de usuários, perfis (`Produtor/Exportador`, `Analista de Dados`, `Administrador`) e auditoria/logs.
* **Monitoramento Agrícola:** Cadastro de culturas/frutas, parâmetros ideais (temperatura e umidade) e lotes de produção.
* **Ingestão de Dados IoT:** Integração direta com sensores físicos (como ESP32/DHT11 via ThingSpeak), evitando duplicidade de dados.
* **Automação de Alertas:** Disparo automático de alertas via triggers quando medições saem das faixas operacionais configuradas.
* **Análise e Previsão:** Modelagem para suporte a previsões de mercado, séries temporais climáticas e geração de relatórios.

---

## Como criar o banco

O script cria o banco (se não existir) e roda os arquivos abaixo, **nesta ordem**, parando no primeiro erro. Todos são idempotentes: rodar de novo não quebra, e em um banco local antigo eles atualizam a estrutura.

1. **`schema.sql`**: tipos `ENUM`, todas as tabelas no estado final e atualização de bancos antigos.
2. **`restricoes.sql`**: `NOT NULL`, `UNIQUE` e `CHECK`.
3. **`seed.sql`**: perfis, frutas, faixas de armazenamento, portos, lote e sensor real (canal 3500765).
4. **`triggers.sql`**: alerta automático na transição para fora da faixa e bloqueio de `DELETE` em `usuario`.
5. **`views.sql`**: views do dashboard (depende de `fn_etapa_lote`, criada em `triggers.sql`).

Datas são gravadas em UTC; o front converte para `America/Recife`.

---

##  Principais Entidades

| Tabela | Descrição |
| :--- | :--- |
| `perfil` | Perfis de acesso do sistema (RBAC). |
| `usuario` | Dados e credenciais de acesso dos usuários. |
| `fruta` | Culturas agrícolas monitoradas (ex: Manga, Uva, Melão). |
| `config_parametro` | Configuração das faixas ideais de temperatura e umidade por fruta. |
| `lote` | Lotes de produção e seu ciclo de vida. |
| `sensor` | Mapeamento dos dispositivos e canais IoT (ex: ThingSpeak `channel_id`). |
| `leitura_climatica` | Histórico de dados coletados (temperatura e umidade) indexados por data e sensor. |
| `alerta` | Ocorrências registradas automaticamente ao violar as faixas ideais. |
| `dados_mercado` | Registros de preços e demandas para exportação. |
| `previsao` / `previsao_climatica` / `previsao_mercado` | Modelagem para integração com modelos preditivos/IA. |
| `log_acesso` | Logs de auditoria das ações dos usuários. |
| `relatorio` | Histórico e metadados de relatórios gerados. |

---

## Funcionalidades e Automações Implementadas

* **Deduplicação da Ingestão ThingSpeak:** Garantida pela constraint `uq_leitura_sensor_entry` (`fk_sensor_id_sensor` + `entry_id_thingspeak`).
* **Soft Delete para Usuários:** Bloqueio de `DELETE` físico na tabela `usuario` via trigger (`fn_bloquear_delete_usuario`), exigindo o uso de alteração de status (`inativo`).
* **Trigger Inteligente de Alerta (`trg_alerta_faixa`):** Detecta transições de faixa ideal e gera alertas automaticamente na transição, evitando poluição de notificações repetitivas a cada leitura.
* **Views de Dashboard (`vw_status_sensores`, `vw_leitura_hora`, `vw_leituras_diarias_lote` e `vw_lote_saude`):** Agregações otimizadas para rápido consumo de interfaces e painéis de controle.

---

## Integração prevista

O banco faz parte de uma arquitetura que poderá integrar diferentes componentes, como:

**Sensores → Coleta de dados → Processamento → Banco de dados → Análise/IA → Alertas → Dashboard**

A estrutura atual serve como base para essa integração e poderá ser modificada conforme os requisitos das demais partes do projeto.

## Tecnologias

* PostgreSQL
* SQL
* Banco de dados relacional

## Contexto

Projeto desenvolvido no contexto acadêmico, como parte de uma solução voltada ao monitoramento e análise de dados relacionados à produção agrícola.
