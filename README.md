# Banco de Dados ValeSafra

Banco de dados desenvolvido para o projeto **ValeSafra**, voltado ao monitoramento climático, acompanhamento de lotes, previsões, alertas e dados relacionados à produção agrícola.

## Status do projeto

> **Em desenvolvimento**

Este banco de dados **ainda está em fase de desenvolvimento e modelagem**. A estrutura apresentada neste repositório **não corresponde à versão final ou geral do banco de dados do projeto**.

As tabelas, relacionamentos, tipos de dados e regras de negócio podem sofrer alterações durante o desenvolvimento da aplicação e a integração com os demais componentes do sistema.

## Objetivo

A estrutura tem como objetivo fornecer uma base para armazenar e relacionar informações como:

* Usuários e seus perfis;
* Frutas e lotes de produção;
* Sensores e suas localizações;
* Leituras de temperatura e umidade;
* Parâmetros climáticos por fruta;
* Dados de mercado;
* Previsões;
* Previsões climáticas e de mercado;
* Alertas;
* Registros de acesso;
* Relatórios.

## Principais entidades

| Tabela               | Descrição                                     |
| -------------------- | --------------------------------------------- |
| `perfil`             | Perfis de acesso dos usuários                 |
| `usuario`            | Dados dos usuários do sistema                 |
| `fruta`              | Cadastro das frutas monitoradas               |
| `lote`               | Lotes de produção                             |
| `sensor`             | Sensores utilizados no monitoramento          |
| `leitura_climatica`  | Registros de temperatura e umidade            |
| `config_parametro`   | Parâmetros climáticos por fruta               |
| `dados_mercado`      | Informações relacionadas ao mercado           |
| `previsao`           | Previsões realizadas pelo sistema             |
| `previsao_climatica` | Relação entre previsões e leituras climáticas |
| `previsao_mercado`   | Relação entre previsões e dados de mercado    |
| `alerta`             | Alertas gerados pelo sistema                  |
| `log_acesso`         | Registro das ações dos usuários               |
| `relatorio`          | Relatórios gerados pelo sistema               |

## Integração

O banco faz parte de uma arquitetura maior do projeto, que poderá envolver:

**Sensores/ESP32 → coleta de dados → processamento → banco de dados → análise/IA → alertas → dashboard**

A estrutura atual serve como uma das bases para essa integração e poderá ser adaptada conforme os requisitos das outras partes do sistema.

## Observação

Este repositório apresenta uma **versão de desenvolvimento** do banco de dados.

Portanto:

* A estrutura ainda pode ser modificada;
* Novas tabelas e relacionamentos podem ser adicionados;
* Campos existentes podem ser alterados;
* Regras de negócio ainda podem ser ajustadas;
* A modelagem atual não deve ser considerada a versão definitiva do banco.

A **versão geral/final** será definida após a conclusão da modelagem e integração com os demais módulos do projeto.

## Tecnologias

* PostgreSQL
* SQL
* Banco de dados relacional

## Desenvolvimento

Projeto acadêmico desenvolvido como parte do projeto **ValeSafra**.
